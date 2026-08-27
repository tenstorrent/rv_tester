# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

import yaml
import logging 
import argparse

class Field:
    def __init__(self, name, width, msb, lsb, legal_value, reset_val, sw_type, perf_val, masked_by=""):
        self.name = name
        self.width = width
        self.range = (msb, lsb)
        self.reset_val = reset_val
        self.legal_value = legal_value
        self.sw_type = sw_type
        self.perf_val = perf_val
        # Field-level MASKED_BY: the gate field inside the CSR-level masking register.
        self.masked_by = masked_by

class CSR:
    def __init__(self, name, address, size, alias_of=""):
        self.name = name
        self.address = address
        self.size = size
        self.field = {}
        self.alias_of = alias_of
        self.reset_val = "0x0"  # Initialize with default value
        self.perf_val = "0x0"  # Initialize with default value
        # CSR-level MASKED_BY: the masking register whose fields gate this CSR.
        self.masked_by_reg = ""
        # Whole-CSR SW_TYPE from common_data (e.g. "Read-Only" on counters).
        self.common_sw_type = ""

    def _field_position_mask(self, field_property):
        """Positioned bit mask for a field, clipped to the CSR size (and 64)."""
        msb, lsb = field_property.range
        mask = 0
        for bit in range(lsb, msb + 1):
            if bit < self.size and bit < 64:
                mask |= 1 << bit
        return mask

    def _resolve_field_value(self, raw_value, field_name, csr_map_instance, resolver_name):
        if not raw_value:
            return 0
        if csr_map_instance:
            return getattr(csr_map_instance, resolver_name)(raw_value, self.name, field_name)
        val_str = str(raw_value).strip()
        try:
            if val_str.lower().startswith("0x"):
                val_str = val_str[2:]
            return int(val_str, 16)
        except (ValueError, TypeError):
            return 0

    def concat_reset_val(self, csr_map_instance=None):
        """Concatenate reset values from all fields in this CSR.

        Field values are masked to the field width and clipped to the CSR
        size, so an over-wide value contributes its in-range bits instead of
        (as the old bitstring implementation did) silently dropping the whole
        field. This keeps the C++ reset, the SV RESET_VAL, and the per-field
        SV RESET consistent with each other."""
        result = 0
        for field_name, field_property in self.field.items():
            resolved = self._resolve_field_value(getattr(field_property, "reset_val", None), field_name, csr_map_instance, "resolve_param_reset_value")
            if resolved:
                lsb = field_property.range[1]
                result |= (resolved << lsb) & self._field_position_mask(field_property)
        self.reset_val = hex(result)
        return self.reset_val

    def concat_perf_val(self, csr_map_instance=None):
        """Concatenate performance values (falling back to reset per field)."""
        result = 0
        for field_name, field_property in self.field.items():
            perf_raw = getattr(field_property, "perf_val", None)
            if perf_raw is not None and str(perf_raw).strip():
                value = self._resolve_field_value(perf_raw, field_name, csr_map_instance, "resolve_param_perf_value")
            else:
                value = self._resolve_field_value(getattr(field_property, "reset_val", None), field_name, csr_map_instance, "resolve_param_reset_value")
            lsb = field_property.range[1]
            mask = self._field_position_mask(field_property)
            result = (result & ~mask) | ((value << lsb) & mask)
        self.perf_val = hex(result)
        return self.perf_val

class CsrMap:
    def __init__(self, csr_spec, csr_param_override=None):
        with open(csr_spec, "r") as c:
            description = yaml.safe_load(c)
        self.csr_property_dict = dict()
        # Load CSR parameter override values from YAML file
        self.cac_check_overrides = {}
        self.csr_params = {}
        self.override_data = {}
        if csr_param_override:
            with open(csr_param_override, "r") as f:
                override_data = yaml.safe_load(f)
                if override_data:
                    self.override_data = override_data
                    if "cac_check_overrides" in override_data:
                        self.cac_check_overrides = override_data["cac_check_overrides"]
                    # Accept both the historical misspelling and the correct one.
                    extra_csrs_keys = [k for k in ("addtional_csrs", "additional_csrs") if override_data.get(k)]
                    if len(extra_csrs_keys) == 2 and override_data["addtional_csrs"] != override_data["additional_csrs"]:
                        raise ValueError("override defines both 'addtional_csrs' and 'additional_csrs' with different content")
                    for key in extra_csrs_keys[:1]:
                        for csr_name, csr_data in override_data[key].items():
                            description[csr_name] = csr_data
                    if "params" in override_data:
                        self.csr_params = override_data["params"]
        for csr_name, csr_data in description.items():
            common_data = csr_data.get("common_data", {})
            csr_address = common_data.get("ADDRESS", "0x0")
            size = int(common_data.get("CSR_SIZE", 64))
            alias_of = common_data.get("ALIAS_OF", "")

            field_list = {k: v for k, v in csr_data.items() if k != "common_data"}
            field_dict = {}

            for field_name, field_data in field_list.items():
                if field_data is None:
                    logging.warning(f"Skipping field {field_name} due to null field_data")
                    continue

                name = field_name
                field_range = field_data.get("FIELDS_RANGE")
                reset_val = field_data.get("RESET_VALUE", "0x0")

                # A missing or malformed FIELDS_RANGE is a spec error, not a fallback.
                field_range = "" if field_range is None else str(field_range)
                try:
                    if ":" in field_range:
                        msb_str, lsb_str = field_range.split(":")
                        msb, lsb = int(msb_str), int(lsb_str)
                    else:
                        msb = lsb = int(field_range)
                except ValueError:
                    raise ValueError(f"CSR '{csr_name}' field '{field_name}': FIELDS_RANGE {field_range!r} is missing or not 'msb:lsb'/'bit'")
                if msb < lsb:
                    raise ValueError(f"CSR '{csr_name}' field '{field_name}': FIELDS_RANGE {field_range!r} has msb < lsb")

                width = msb - lsb + 1
                # No LEGAL_VALUE means unconstrained, not "only 0 is legal".
                legal_value = field_data.get("LEGAL_VALUE") or ""
                sw_type = field_data.get("SW_TYPE", "WARL")
                if not sw_type:
                    sw_type = "WARL"
                perf_val = field_data.get("PERF_VALUE", None)
                masked_by = str(field_data.get("MASKED_BY") or "").strip()

                field_obj = Field(field_name, width, msb, lsb, legal_value, reset_val, sw_type, perf_val, masked_by)
                field_obj.mask_writes_only = str(field_data.get("MASK_WRITES_ONLY?") or "").strip()
                field_dict[field_name] = field_obj

            csr_obj = CSR(csr_name, csr_address, size, alias_of)
            csr_obj.field = field_dict
            csr_obj.masked_by_reg = str(common_data.get("MASKED_BY") or "").strip()
            csr_obj.common_sw_type = str(common_data.get("SW_TYPE") or "").strip()
            self.csr_property_dict[csr_name] = csr_obj

    def resolve_param_reset_value(self, reset_val, csr_name, field_name):
        """Helper method to resolve PARAM reset values by looking up in csr_params"""
        if not reset_val:
            return 0
        
        reset_str = str(reset_val).strip()
        
        # Check if the value is PARAM and we have CSR and field names
        if reset_str.upper() == "PARAM" and csr_name and field_name:
            # Construct parameter name: CSR_<CSR_NAME.uppercase()>_F_<field_name.uppercase()>_RESET_VALUE
            param_name = f"CSR_{csr_name.replace('_','').upper()}_F_{field_name.replace('-', '_').upper()}_RESET_VALUE"
            # Look up in shared csr_params dictionary
            if self.csr_params and param_name in self.csr_params:
                param_value = self.csr_params[param_name]
                # Convert parameter value to integer
                try:
                    if isinstance(param_value, str):
                        param_str = param_value.strip()
                        if param_str.lower().startswith("0x"):
                            param_str = param_str[2:]
                        return int(param_str, 16)
                    else:
                        return int(param_value)
                except (ValueError, TypeError):
                    return 0
            else:
                # Parameter not found, return 0
                return 0
        
        # Not a PARAM value, try to parse as hex
        if reset_str.lower().startswith("0x"):
            reset_str = reset_str[2:]
        
        try:
            return int(reset_str, 16)
        except ValueError:
            return 0

    def resolve_param_perf_value(self, perf_val, csr_name, field_name):
        """Helper method to resolve PARAM performance values by looking up in csr_params"""
        if not perf_val:
            return 0
        
        perf_str = str(perf_val).strip()
        
        # Check if the value is PARAM and we have CSR and field names
        if perf_str.upper() == "PARAM" and csr_name and field_name:
            # Construct parameter name: CSR_<CSR_NAME.uppercase()>_F_<field_name.uppercase()>_PERF_VALUE
            param_name = f"CSR_{csr_name.replace('_','').upper()}_F_{field_name.replace('-', '_').upper()}_PERF_VALUE"
            # Look up in shared csr_params dictionary (contains both reset and perf params)
            if self.csr_params and param_name in self.csr_params:
                param_value = self.csr_params[param_name]
                # Convert parameter value to integer
                try:
                    if isinstance(param_value, str):
                        param_str = param_value.strip()
                        if param_str.lower().startswith("0x"):
                            param_str = param_str[2:]
                        return int(param_str, 16)
                    else:
                        return int(param_value)
                except (ValueError, TypeError):
                    return 0
            else:
                # Parameter not found, return 0
                return 0
        
        # Not a PARAM value, try to parse as hex
        if perf_str.lower().startswith("0x"):
            perf_str = perf_str[2:]
        
        try:
            return int(perf_str, 16)
        except ValueError:
            return 0

    def generate_hpp_file(self, output_file):
        """Generate C++ header file with CSR structures"""
        def sanitize_name(name):
            """Sanitize names to be valid C++ identifiers"""
            sanitized = name.replace("-", "_").replace(".", "_").replace(" ","_")
            if sanitized and sanitized[0].isdigit():
                sanitized = "_" + sanitized
            cpp_keywords = {
                "class", "struct", "int", "char", "float", "double", "void", 
                "if", "else", "for", "while", "do", "switch", "case", "default",
                "break", "continue", "return", "goto", "sizeof", "typedef",
                "static", "extern", "auto", "register", "const", "volatile",
                "signed", "unsigned", "enum", "union", "namespace", "using",
                "template", "typename", "public", "private", "protected",
                "virtual", "inline", "friend", "operator", "new", "delete",
                "this", "try", "catch", "throw", "true", "false", "nullptr",
                "time"
            }
            if sanitized.lower() in cpp_keywords:
                sanitized += "_"
            return sanitized

        def parse_hex_address(address):
            """Convert address to 12-bit hex value (uint64_t)"""
            if not address:
                return 0
            
            addr_str = str(address).strip()
            
            if addr_str.lower().startswith("0x"):
                addr_str = addr_str[2:]
            
            try:
                addr_val = int(addr_str, 16) & 0xFFF
                return addr_val
            except ValueError:
                return 0

        def parse_hex_reset_val(reset_val, csr_name=None, field_name=None):
            """Convert reset value to 64-bit hex value (uint64_t)"""
            reset_value = self.resolve_param_reset_value(reset_val, csr_name, field_name)
            return reset_value & 0xFFFFFFFFFFFFFFFF

        def parse_hex_legal_values(legal_value):
            """Convert legal value string to array of 64-bit hex values (vector<uint64_t>)"""
            if not legal_value:
                return []
            
            legal_str = str(legal_value).strip()
            
            if not legal_str:
                return []
            
            values = []
            for val_str in legal_str.split(','):
                val_str = val_str.strip()
                if not val_str:
                    continue
                
                if val_str.lower().startswith("0x"):
                    val_str = val_str[2:]
                
                try:
                    val = int(val_str, 16) & 0xFFFFFFFFFFFFFFFF
                    values.append(val)
                except ValueError:
                    continue
            
            return values

        def parse_hex_perf_val(perf_val, csr_name=None, field_name=None):
            """Convert perf value to 64-bit hex value (uint64_t)"""
            perf_value = self.resolve_param_perf_value(perf_val, csr_name, field_name)
            return perf_value & 0xFFFFFFFFFFFFFFFF

        with open(output_file, 'w') as f:
            f.write("#pragma once\n")
            f.write("#include <string>\n")
            f.write("#include <utility>\n")
            f.write("#include <cstdint>\n")
            f.write("#include <vector>\n\n")
            
            f.write("namespace CSR {\n\n")
            
            f.write("struct field {\n")
            f.write("    std::string name;\n")
            f.write("    int width;\n")
            f.write("    std::pair<int, int> range;  // (msb, lsb)\n")
            f.write("    std::uint64_t reset_val;  // 64-bit hex reset value\n")
            f.write("    std::vector<std::uint64_t> legal_value;  // Array of 64-bit hex legal values\n")
            f.write("    std::string sw_type;\n")
            f.write("    std::string description;\n")
            f.write("    std::uint64_t bit_mask;  // Bit position as hex number (bits set to 1 for field positions)\n")
            f.write("    std::uint64_t perf_val;\n")
            f.write("    \n")
            f.write("    inline std::uint64_t extract_value(std::uint64_t csr_data) const {\n")
            f.write("        return (csr_data & bit_mask) >> range.second;\n")
            f.write("    }\n")
            f.write("};\n\n")
            
            f.write("struct csr_base {\n")
            f.write("    std::string name;\n")
            f.write("    std::uint64_t address;  // 12-bit hex address\n")
            f.write("    int size;\n")
            f.write("    csr_base* alias_of;  // Pointer to aliased CSR struct\n")
            f.write("    std::uint64_t reset_val;  // 64-bit field-based concatenated reset value\n")
            f.write("    std::uint64_t perf_val;\n")
            f.write("    bool cac_check;  // CAC check enable flag\n")
            f.write("    \n")
            f.write("    csr_base(const std::string& csr_name, std::uint64_t csr_address, int csr_size, bool csr_cac_check = true)\n")
            f.write("        : name(csr_name), address(csr_address), size(csr_size), alias_of(nullptr), reset_val(0), perf_val(0), cac_check(csr_cac_check) {}\n")
            f.write("    \n")
            f.write("    csr_base() = default;\n")
            f.write("};\n\n")
            
            for csr_name, csr in self.csr_property_dict.items():
                sanitized_csr_name = sanitize_name(csr_name).lower()
                struct_name = f"{sanitized_csr_name}_csr"
                
                f.write(f"struct {struct_name} : public csr_base {{\n")
                
                hex_address = parse_hex_address(csr.address)
                lowercase_csr_name = csr_name.lower()
                # Calculate the concatenated reset value for this CSR
                csr_reset_val = csr.concat_reset_val(self)
                csr_reset_val_int = 0
                if csr_reset_val:
                    reset_str = str(csr_reset_val).strip()
                    if reset_str.lower().startswith("0x"):
                        reset_str = reset_str[2:]
                    try:
                        csr_reset_val_int = int(reset_str, 16) & 0xFFFFFFFFFFFFFFFF
                    except ValueError:
                        csr_reset_val_int = 0
                # Calculate the concatenated perf value for this CSR
                csr_perf_val = csr.concat_perf_val(self)
                csr_perf_val_int = 0
                if csr_perf_val:
                    perf_str = str(csr_perf_val).strip()
                    if perf_str.lower().startswith("0x"):
                        perf_str = perf_str[2:]
                    try:
                        csr_perf_val_int = int(perf_str, 16) & 0xFFFFFFFFFFFFFFFF
                    except ValueError:
                        csr_perf_val_int = 0

                # Determine cac_check value:
                # - Default false for CSRs starting with "c_"
                # - Default true for all other CSRs
                # - Override YAML can override any of them
                default_cac_check = not csr_name.lower().startswith("c_")
                cac_check_val = self.cac_check_overrides.get(csr_name, default_cac_check)
                cac_check_str = "true" if cac_check_val else "false"

                f.write(f"    {struct_name}() : csr_base(\"{lowercase_csr_name}\", 0x{hex_address:03X}, {csr.size}, {cac_check_str}) {{\n")
                f.write(f"        reset_val = 0x{csr_reset_val_int:016X}ULL;\n")
                f.write(f"        perf_val = 0x{csr_perf_val_int:016X}ULL;\n")
                f.write(f"    }}\n\n")
                
                for field_name, field in csr.field.items():
                    sanitized_field_name = sanitize_name(field_name).upper()
                    uppercase_field_name = field_name.upper()
                    field_full_name = f"{lowercase_csr_name}.{uppercase_field_name}"
                    msb, lsb = field.range
                    width_val = field.width if field.width else "0"
                    reset_val = parse_hex_reset_val(field.reset_val, csr_name, field_name)
                    legal_values = parse_hex_legal_values(field.legal_value)
                    perf_val = parse_hex_perf_val(field.perf_val, csr_name, field_name)
                    
                    f.write(f"    field {sanitized_field_name} = {{\n")
                    f.write(f"        \"{field_full_name}\",\n")
                    f.write(f"        {width_val},\n")
                    f.write(f"        {{{msb}, {lsb}}},\n")
                    f.write(f"        0x{reset_val:016X}ULL,\n")
                    if legal_values:
                        legal_values_str = ', '.join([f'0x{val:016X}ULL' for val in legal_values])
                        f.write(f"        {{{legal_values_str}}},\n")
                    else:
                        f.write(f"        {{}},\n")
                    f.write(f"        \"{field.sw_type}\",\n")
                    f.write(f"        \"\",\n")
                    
                    msb, lsb = field.range
                    bit_pos_value = 0
                    for bit_pos in range(lsb, msb + 1):
                        if bit_pos < csr.size and bit_pos < 64:
                            bit_pos_value |= (1 << bit_pos)
                    f.write(f"        0x{bit_pos_value:016X}ULL,\n")
                    f.write(f"        0x{perf_val:016X}ULL\n")
                    f.write(f"    }};\n")
                
                f.write(f"}};\n\n")
            
            f.write("// Global CSR instances\n")
            for csr_name in self.csr_property_dict.keys():
                sanitized_csr_name = sanitize_name(csr_name).lower()
                struct_name = f"{sanitized_csr_name}_csr"
                instance_name = sanitized_csr_name
                f.write(f"extern {struct_name} {instance_name};\n")
            
            f.write("\n")
            
            f.write("// CSR instance definitions\n")
            for csr_name in self.csr_property_dict.keys():
                sanitized_csr_name = sanitize_name(csr_name).lower()
                struct_name = f"{sanitized_csr_name}_csr"
                instance_name = sanitized_csr_name
                f.write(f"inline {struct_name} {instance_name};\n")
            f.write("\n")
            
            f.write("// Vector containing all CSR instances\n")
            f.write("extern std::vector<csr_base*> csr_map;\n")
            f.write("\n")
            
            f.write("// Vector definition with all CSR instances\n")
            f.write("inline std::vector<csr_base*> csr_map = {\n")
            instance_names = []
            for csr_name in self.csr_property_dict.keys():
                sanitized_csr_name = sanitize_name(csr_name).lower()
                instance_name = sanitized_csr_name
                instance_names.append(f"    &{instance_name}")
            f.write(",\n".join(instance_names))
            f.write("\n};\n\n")
            
            f.write("// Utility functions for CSR management\n")
            f.write("inline size_t get_csr_count() {\n")
            f.write("    return csr_map.size();\n")
            f.write("}\n\n")
            
            f.write("inline csr_base* find_csr_by_name(const std::string& name) {\n")
            f.write("    for (auto* csr : csr_map) {\n")
            f.write("        if (csr->name == name) {\n")
            f.write("            return csr;\n")
            f.write("        }\n")
            f.write("    }\n")
            f.write("    return nullptr;\n")
            f.write("}\n\n")
            
            f.write("inline csr_base* find_csr_by_address(std::uint64_t address) {\n")
            f.write("    for (auto* csr : csr_map) {\n")
            f.write("        if (csr->address == address) {\n")
            f.write("            return csr;\n")
            f.write("        }\n")
            f.write("    }\n")
            f.write("    return nullptr;\n")
            f.write("}\n\n")
            
            f.write("// Function to initialize alias pointers\n")
            f.write("// Call this after all CSR instances are created to set up CSR aliases\n")
            f.write("inline void initialize_csr_aliases() {\n")
            
            for csr_name, csr in self.csr_property_dict.items():
                if csr.alias_of:
                    sanitized_csr_name = sanitize_name(csr_name).lower()
                    instance_name = sanitized_csr_name
                    
                    alias_csr_name = sanitize_name(csr.alias_of).lower()
                    alias_instance_name = alias_csr_name
                    
                    f.write(f"    {instance_name}.alias_of = &{alias_instance_name};\n")
            
            f.write("}\n\n")
            
            f.write("} // namespace CSR\n")

        print(f"Generated C++ header file: {output_file}")

    def generate_sv_file(self, output_file):
        """Generate SystemVerilog defines file with CSR structures"""
        def sanitize_sv_name(name):
            """Sanitize names to be valid SystemVerilog identifiers"""
            sanitized = name.replace("-", "_").replace(".", "_").replace(" ", "_").upper()
            if sanitized and sanitized[0].isdigit():
                sanitized = "_" + sanitized
            return sanitized

        def format_hex_address(address):
            """Convert address to proper hex format for SystemVerilog"""
            if not address:
                return "12'h000"
            
            addr_str = str(address).strip()
            if addr_str.lower().startswith("0x"):
                addr_str = addr_str[2:]
            
            try:
                addr_val = int(addr_str, 16) & 0xFFF
                return f"12'h{addr_val:03X}"
            except ValueError:
                return "12'h000"

        def format_hex_reset_val(reset_val, csr_name=None, field_name=None):
            """Convert reset value to proper hex format for SystemVerilog"""
            reset_value = self.resolve_param_reset_value(reset_val, csr_name, field_name)
            reset_value = reset_value & 0xFFFFFFFFFFFFFFFF
            return f"64'h{reset_value:016X}"

        with open(output_file, 'w') as f:
            f.write("// SystemVerilog CSR Defines Package\n")
            f.write("// Auto-generated from CSR specification\n\n")
            
            f.write("package csr_map_pkg;\n\n")
            
            # Generate CSR address defines
            f.write("// CSR Address Defines\n")
            for csr_name, csr in self.csr_property_dict.items():
                sanitized_csr_name = sanitize_sv_name(csr_name)
                hex_address = format_hex_address(csr.address)
                f.write(f"parameter logic [11:0] {sanitized_csr_name}_ADDR = {hex_address};\n")
            f.write("\n")
            
            # Generate CSR size defines
            f.write("// CSR Size Defines\n")
            for csr_name, csr in self.csr_property_dict.items():
                sanitized_csr_name = sanitize_sv_name(csr_name)
                f.write(f"parameter int {sanitized_csr_name}_SIZE = {csr.size};\n")
            f.write("\n")
            
            # Generate field defines for each CSR
            for csr_name, csr in self.csr_property_dict.items():
                sanitized_csr_name = sanitize_sv_name(csr_name)
                f.write(f"// {csr_name.upper()} CSR Field Defines\n")
                
                for field_name, field in csr.field.items():
                    sanitized_field_name = sanitize_sv_name(field_name)
                    prefix = f"{sanitized_csr_name}_{sanitized_field_name}"
                    
                    msb, lsb = field.range
                    width = field.width if field.width else 0
                    reset_val = format_hex_reset_val(field.reset_val, csr_name, field_name)
                    
                    # Field range defines
                    f.write(f"parameter int {prefix}_MSB = {msb};\n")
                    f.write(f"parameter int {prefix}_LSB = {lsb};\n")
                    f.write(f"parameter int {prefix}_WIDTH = {width};\n")
                    
                    # Field reset value, masked to the field width.
                    field_reset_val = self.resolve_param_reset_value(field.reset_val, csr_name, field_name) & ((1 << width) - 1)

                    f.write(f"parameter logic [{width-1}:0] {prefix}_RESET = {width}'h{field_reset_val:0{(width+3)//4}X};\n")

                    # Field bit mask (for extracting field from full CSR value)
                    bit_mask = 0
                    for bit_pos in range(lsb, msb + 1):
                        if bit_pos < csr.size and bit_pos < 64:
                            bit_mask |= (1 << bit_pos)
                    f.write(f"parameter logic [63:0] {prefix}_MASK = 64'h{bit_mask:016X};\n")
                    
                    # SW type as string parameter
                    f.write(f"parameter string {prefix}_SW_TYPE = \"{field.sw_type}\";\n")
                    
                    f.write("\n")
                
                f.write("\n")
            
            # Generate CSR reset value defines
            f.write("// CSR Reset Value Defines\n")
            for csr_name, csr in self.csr_property_dict.items():
                sanitized_csr_name = sanitize_sv_name(csr_name)
                
                # Calculate full CSR reset value by combining all field reset values
                full_reset_val = 0
                for field_name, field in csr.field.items():
                    msb, lsb = field.range
                    field_reset = self.resolve_param_reset_value(field.reset_val, csr_name, field_name)
                    
                    # Mask field reset value to field width and position it
                    field_width = msb - lsb + 1
                    field_mask = (1 << field_width) - 1
                    field_reset = (field_reset & field_mask) << lsb
                    full_reset_val |= field_reset
                
                f.write(f"parameter logic [63:0] {sanitized_csr_name}_RESET_VAL = 64'h{full_reset_val:016X};\n")
            
            f.write("\n")
            
            # Generate utility macros for field access
            f.write("// Utility Macros for Field Access\n")
            f.write("// Extract field value from CSR value\n")
            f.write("`define CSR_FIELD_GET(csr_val, field_msb, field_lsb) \\\n")
            f.write("    ((csr_val >> field_lsb) & ((1 << (field_msb - field_lsb + 1)) - 1))\n\n")
            
            f.write("// Set field value in CSR value\n")
            f.write("`define CSR_FIELD_SET(csr_val, field_val, field_msb, field_lsb) \\\n")
            f.write("    ((csr_val & ~(((1 << (field_msb - field_lsb + 1)) - 1) << field_lsb)) | \\\n")
            f.write("     ((field_val & ((1 << (field_msb - field_lsb + 1)) - 1)) << field_lsb))\n\n")
            
            # Generate field access macros for each CSR field
            f.write("// Field Access Macros\n")
            for csr_name, csr in self.csr_property_dict.items():
                sanitized_csr_name = sanitize_sv_name(csr_name)
                
                for field_name, field in csr.field.items():
                    sanitized_field_name = sanitize_sv_name(field_name)
                    prefix = f"{sanitized_csr_name}_{sanitized_field_name}"
                    
                    msb, lsb = field.range
                    
                    # Macro to get field value from CSR
                    f.write(f"`define {prefix}_GET(csr_val) \\\n")
                    f.write(f"    `CSR_FIELD_GET(csr_val, {msb}, {lsb})\n\n")
                    
                    # Macro to set field value in CSR
                    f.write(f"`define {prefix}_SET(csr_val, field_val) \\\n")
                    f.write(f"    `CSR_FIELD_SET(csr_val, field_val, {msb}, {lsb})\n\n")
            
            # Generate alias information if present
            alias_csrs = [(name, csr) for name, csr in self.csr_property_dict.items() if csr.alias_of]
            if alias_csrs:
                f.write("// CSR Alias Defines\n")
                for csr_name, csr in alias_csrs:
                    sanitized_csr_name = sanitize_sv_name(csr_name)
                    alias_csr_name = sanitize_sv_name(csr.alias_of)
                    f.write(f"// {csr_name} is an alias of {csr.alias_of}\n")
                    f.write(f"parameter logic [11:0] {sanitized_csr_name}_ALIAS_OF_ADDR = {alias_csr_name}_ADDR;\n")
                f.write("\n")
            
            f.write("endpackage : csr_map_pkg\n")

        print(f"Generated SystemVerilog defines file: {output_file}")


# ============================================================================
# CSRAL: generated tables for the runtime CSR model; validation fails the
# build, never a simulation. See docs/source/user_guides/cosim.rst.
# ============================================================================

import fnmatch


class CsralValidationError(ValueError):
    pass


_ALLOWED_CSRAL_KEYS = {"compare_mask_source", "reset_check", "field_aliases", "policies"}
_ALLOWED_POLICY_KEYS = {"check", "on_mismatch", "volatile", "class", "may_not_exist", "exists_if", "check_reset"}
_ON_MISMATCH_VALUES = ("error", "skip", "resynch_rd")
_COMPARE_MASK_SOURCES = ("whisper", "spec", "both_warn")
_RESET_CHECK_VALUES = ("error", "warn", "off")


class CsralConfig:
    """csral policy configuration: rv_tester defaults merged with the
    project's `csral:` section (project wins per key/pattern)."""

    def __init__(self, defaults_yaml_path, project_override_data):
        with open(defaults_yaml_path, "r") as f:
            defaults = yaml.safe_load(f) or {}
        project = (project_override_data or {}).get("csral") or {}

        for origin, data in (("defaults", defaults), ("project override", project)):
            unknown = set(data.keys()) - _ALLOWED_CSRAL_KEYS
            if "save_restore" in unknown:
                raise CsralValidationError(f"csral {origin}: 'save_restore' is not a YAML knob; the enabled set is runtime-only via +csral_save_restore")
            if unknown:
                raise CsralValidationError(f"csral {origin}: unknown keys {sorted(unknown)} (allowed: {sorted(_ALLOWED_CSRAL_KEYS)})")

        self.compare_mask_source = project.get("compare_mask_source", defaults.get("compare_mask_source", "whisper"))
        if self.compare_mask_source not in _COMPARE_MASK_SOURCES:
            raise CsralValidationError(f"csral: compare_mask_source {self.compare_mask_source!r} not in {_COMPARE_MASK_SOURCES}")
        self.reset_check = project.get("reset_check", defaults.get("reset_check", "error"))
        if self.reset_check not in _RESET_CHECK_VALUES:
            raise CsralValidationError(f"csral: reset_check {self.reset_check!r} not in {_RESET_CHECK_VALUES}")

        # field_aliases: defaults plus project additions (deduplicated).
        self.field_aliases = list(defaults.get("field_aliases") or [])
        for fa in project.get("field_aliases") or []:
            if fa not in self.field_aliases:
                self.field_aliases.append(fa)

        # policies: {pattern: (policy_dict, origin)}. Project wins per pattern.
        self.policies = {}
        for origin, data in (("defaults", defaults), ("project", project)):
            for pattern, pol in (data.get("policies") or {}).items():
                pol = dict(pol or {})
                unknown = set(pol.keys()) - _ALLOWED_POLICY_KEYS
                if unknown:
                    raise CsralValidationError(f"csral policy {pattern!r} ({origin}): unknown keys {sorted(unknown)} (allowed: {sorted(_ALLOWED_POLICY_KEYS)})")
                if pol.get("on_mismatch", "error") not in _ON_MISMATCH_VALUES:
                    raise CsralValidationError(f"csral policy {pattern!r} ({origin}): on_mismatch {pol.get('on_mismatch')!r} not in {_ON_MISMATCH_VALUES}")
                if pol.get("class") not in (None, "interrupt"):
                    raise CsralValidationError(f"csral policy {pattern!r} ({origin}): class {pol.get('class')!r} (only 'interrupt' is defined)")
                self.policies[str(pattern)] = (pol, origin)

    def resolve_policy(self, csr_name):
        """Exact name beats glob beats 'default'. Two different globs matching
        the same CSR with different policies is ambiguous -> error."""
        if csr_name in self.policies:
            return dict(self.policies[csr_name][0])
        glob_hits = [(pat, pol) for pat, (pol, _origin) in self.policies.items()
                     if pat != "default" and pat != csr_name and _is_glob(pat) and fnmatch.fnmatchcase(csr_name, pat)]
        if glob_hits:
            first = glob_hits[0][1]
            for pat, pol in glob_hits[1:]:
                if pol != first:
                    raise CsralValidationError(f"csral policies: CSR '{csr_name}' matches globs {sorted(p for p, _ in glob_hits)} with different policies; add an exact entry to disambiguate")
            return dict(first)
        if "default" in self.policies:
            return dict(self.policies["default"][0])
        return {}


def _is_glob(pattern):
    return any(ch in pattern for ch in "*?[")


_READONLY_SW_TYPES = {"read-only", "ro", "wpri"}


class CsralModel:
    """Derived, validated model behind csral_tables.hpp."""

    def __init__(self, csr_map, config):
        self.csr_map = csr_map
        self.config = config
        self.errors = []
        self.warnings = []
        # Ordered rows, spec order (dict order is insertion order).
        self.csr_names = list(csr_map.csr_property_dict.keys())
        self.csr_index = {n: i for i, n in enumerate(self.csr_names)}
        self._validate_spec()
        self._resolve_policies()
        self._derive_conditions()
        self._resolve_exists_if()
        self._resolve_field_aliases()
        self._check_override_names()
        for w in self.warnings:
            logging.warning(f"csral: {w}")
        if self.errors:
            msg = "\n  ".join(self.errors)
            raise CsralValidationError(f"csral validation failed ({len(self.errors)} error(s)):\n  {msg}")

    # Empty ADDRESS = indirect-only CSR: this sentinel, no address-index slot.
    NO_DIRECT_ADDRESS = 0xFFFF

    # -- helpers -------------------------------------------------------------
    def _addr(self, csr):
        addr_str = str(csr.address or "").strip()
        if not addr_str:
            return self.NO_DIRECT_ADDRESS
        if addr_str.lower().startswith("0x"):
            addr_str = addr_str[2:]
        try:
            return int(addr_str, 16) & 0xFFF
        except ValueError:
            self.errors.append(f"CSR '{csr.name}': ADDRESS {csr.address!r} is not a hex address")
            return 0

    def _field_mask(self, csr, field):
        msb, lsb = field.range
        mask = 0
        for bit in range(lsb, msb + 1):
            if bit < csr.size and bit < 64:
                mask |= 1 << bit
        return mask

    # -- validation ----------------------------------------------------------
    def _validate_spec(self):
        seen_addr = {}
        seen_param_keys = {}
        for name, csr in self.csr_map.csr_property_dict.items():
            if csr.size > 64 or csr.size <= 0:
                self.errors.append(f"CSR '{name}': CSR_SIZE {csr.size} unsupported (must be 1..64)")
                continue
            addr = self._addr(csr)
            if addr != self.NO_DIRECT_ADDRESS:
                if addr in seen_addr:
                    self.errors.append(f"CSR '{name}': address 0x{addr:03X} already used by '{seen_addr[addr]}'")
                else:
                    seen_addr[addr] = name
            if csr.alias_of and csr.alias_of not in self.csr_map.csr_property_dict:
                self.errors.append(f"CSR '{name}': ALIAS_OF '{csr.alias_of}' is not a CSR in this spec")

            used_bits = 0
            for fname, field in csr.field.items():
                msb, lsb = field.range
                if msb >= csr.size:
                    self.errors.append(f"CSR '{name}' field '{fname}': range [{msb}:{lsb}] exceeds CSR_SIZE {csr.size}")
                fmask = self._field_mask(csr, field)
                if used_bits & fmask:
                    self.errors.append(f"CSR '{name}' field '{fname}': range [{msb}:{lsb}] overlaps another field")
                used_bits |= fmask
                reset = self.csr_map.resolve_param_reset_value(field.reset_val, name, fname)
                if field.width < 64 and reset >> field.width:
                    self.errors.append(f"CSR '{name}' field '{fname}': reset value 0x{reset:X} does not fit {field.width} bit(s)")
                if str(field.reset_val).strip().upper() == "PARAM":
                    key = f"CSR_{name.replace('_','').upper()}_F_{fname.replace('-', '_').upper()}_RESET_VALUE"
                    if key in seen_param_keys and seen_param_keys[key] != (name, fname):
                        other = seen_param_keys[key]
                        self.errors.append(f"PARAM key collision: '{name}.{fname}' and '{other[0]}.{other[1]}' both mangle to {key}")
                    seen_param_keys[key] = (name, fname)
                if getattr(field, "mask_writes_only", ""):
                    self.errors.append(f"CSR '{name}' field '{fname}': MASK_WRITES_ONLY? is set but CSRAL does not implement write-only masking yet")

    def _resolve_policies(self):
        self.policy_by_csr = {}
        for name in self.csr_names:
            pol = self.config.resolve_policy(name)
            # cac_check_overrides keeps working, mapped onto 'check'.
            if name in self.csr_map.cac_check_overrides:
                pol["check"] = bool(self.csr_map.cac_check_overrides[name])
            self.policy_by_csr[name] = {
                "check": bool(pol.get("check", True)),
                "on_mismatch": pol.get("on_mismatch", "error"),
                "volatile": bool(pol.get("volatile", False)),
                "interrupt": pol.get("class") == "interrupt",
                "may_not_exist": bool(pol.get("may_not_exist", False)),
                # False exempts the CSR from the whisper-vs-spec reset check.
                "check_reset": bool(pol.get("check_reset", True)),
                # Whole-CSR existence gate; resolved to a condition index later.
                "exists_if": str(pol.get("exists_if") or ""),
            }

    def _resolve_exists_if(self):
        """Map each policy's exists_if condition name to a condition index.

        The name must be a condition DERIVED from this spec's MASKED_BY data;
        a dangling name is an error when the project set it and a warning
        when it came from the shared defaults (projects have different CSR
        sets, so e.g. exists_if: misa.H is meaningless for a core without a
        hypervisor gate)."""
        cond_index = {c["name"]: i for i, c in enumerate(self.conditions)}
        for name, pol in self.policy_by_csr.items():
            cond_name = pol.pop("exists_if", "")
            pol["exists_if_index"] = -1
            if not cond_name:
                continue
            if cond_name in cond_index:
                pol["exists_if_index"] = cond_index[cond_name]
            else:
                # Attribute severity to whoever supplied the winning pattern.
                origin = "defaults"
                if name in self.config.policies:
                    origin = self.config.policies[name][1]
                else:
                    for pattern, (_pol2, pat_origin) in self.config.policies.items():
                        if pattern != "default" and _is_glob(pattern) and fnmatch.fnmatchcase(name, pattern) and pat_origin != "defaults":
                            origin = pat_origin
                            break
                msg = f"csral policy for '{name}': exists_if '{cond_name}' is not a condition derived from this spec"
                (self.warnings if origin == "defaults" else self.errors).append(msg)

    def _check_override_names(self):
        names = set(self.csr_names)
        # Unresolved policy patterns: project override errors, shared defaults warn.
        for pattern, (_pol, origin) in self.config.policies.items():
            if pattern == "default":
                continue
            hit = pattern in names if not _is_glob(pattern) else any(fnmatch.fnmatchcase(n, pattern) for n in names)
            if not hit:
                msg = f"csral policy pattern {pattern!r} matches no CSR in this spec"
                (self.warnings if origin == "defaults" else self.errors).append(msg)
        # An unknown cac_check_overrides name is an error (a typo disables nothing).
        for name in self.csr_map.cac_check_overrides:
            if name not in names:
                self.errors.append(f"cac_check_overrides: '{name}' is not a CSR in this spec")

    # -- masked-by conditions --------------------------------------------------
    def _derive_conditions(self):
        # (gate_reg_name, gate_field_key) -> {target_csr_name: mask}
        cond_targets = {}
        for name, csr in self.csr_map.csr_property_dict.items():
            if csr.masked_by_reg and not any(f.masked_by for f in csr.field.values()):
                self.warnings.append(f"CSR '{name}': common_data MASKED_BY '{csr.masked_by_reg}' but no field names a gate field; no masking derived")
            for fname, field in csr.field.items():
                if not field.masked_by:
                    continue
                gate_reg = csr.masked_by_reg
                if not gate_reg:
                    self.errors.append(f"CSR '{name}' field '{fname}': field-level MASKED_BY '{field.masked_by}' but the CSR has no common_data MASKED_BY naming the masking register")
                    continue
                gate_csr = self.csr_map.csr_property_dict.get(gate_reg)
                if gate_csr is None:
                    msg = f"CSR '{name}': common_data MASKED_BY '{gate_reg}' is not a CSR in this spec"
                    if msg not in self.errors:
                        self.errors.append(msg)
                    continue
                if field.masked_by not in gate_csr.field:
                    self.errors.append(f"CSR '{name}' field '{fname}': MASKED_BY '{field.masked_by}' is not a field of masking register '{gate_reg}'")
                    continue
                key = (gate_reg, field.masked_by)
                cond_targets.setdefault(key, {}).setdefault(name, 0)
                cond_targets[key][name] |= self._field_mask(csr, field)

        # Deterministic order: by gate register spec order, then field key.
        self.conditions = []
        for (gate_reg, gate_field) in sorted(cond_targets.keys(), key=lambda k: (self.csr_index.get(k[0], 1 << 30), k[1])):
            targets = cond_targets[(gate_reg, gate_field)]
            gate_csr = self.csr_map.csr_property_dict[gate_reg]
            gate_mask = self._field_mask(gate_csr, gate_csr.field[gate_field])
            view_only = all(bool(self.csr_map.csr_property_dict[t].alias_of) for t in targets)
            self.conditions.append({
                "name": f"{gate_reg}.{gate_field}",
                "gate_csr": gate_reg,
                "gate_mask": gate_mask,
                "view_only": view_only,
                "targets": [(t, targets[t]) for t in self.csr_names if t in targets],
            })

        # The masked-by gate graph must be a DAG.
        edges = {}
        for cond in self.conditions:
            for target, _mask in cond["targets"]:
                edges.setdefault(target, set()).add(cond["gate_csr"])
        state = {}

        def visit(node, stack):
            state[node] = 1
            for gate in edges.get(node, ()):  # target depends on its gate
                if state.get(gate) == 1:
                    self.errors.append(f"masked-by cycle: {' -> '.join(stack + [node, gate])}")
                    return
                if state.get(gate) is None:
                    visit(gate, stack + [node])
            state[node] = 2

        for node in list(edges):
            if state.get(node) is None:
                visit(node, [])

    def _resolve_field_aliases(self):
        self.field_alias_rows = []
        for fa in self.config.field_aliases:
            missing = {"csr", "bits", "alias_of", "alias_bits"} - set(fa.keys())
            if missing:
                self.errors.append(f"csral field_aliases entry {fa!r}: missing keys {sorted(missing)}")
                continue

            def parse_bits(spec):
                s = str(spec)
                if ":" in s:
                    m, l = s.split(":")
                    return int(m), int(l)
                return int(s), int(s)

            try:
                msb, lsb = parse_bits(fa["bits"])
                amsb, alsb = parse_bits(fa["alias_bits"])
            except ValueError:
                self.errors.append(f"csral field_aliases entry {fa!r}: bits must be 'msb:lsb'")
                continue
            for which, cname in (("csr", fa["csr"]), ("alias_of", fa["alias_of"])):
                if cname not in self.csr_map.csr_property_dict:
                    # Shared defaults only warn on a missing CSR.
                    self.warnings.append(f"csral field_aliases: {which} '{cname}' is not a CSR in this spec; entry {fa!r} skipped")
                    break
            else:
                if msb - lsb != amsb - alsb:
                    self.errors.append(f"csral field_aliases entry {fa!r}: widths differ")
                    continue
                self.field_alias_rows.append((fa["csr"], msb, lsb, fa["alias_of"], amsb, alsb))

    # -- emission ---------------------------------------------------------------
    def spec_write_mask(self, csr):
        if csr.common_sw_type.lower() in _READONLY_SW_TYPES:
            return 0
        mask = 0
        for field in csr.field.values():
            if str(field.sw_type).lower() in _READONLY_SW_TYPES:
                continue
            mask |= self._field_mask(csr, field)
        return mask

    def generate_tables_hpp(self, output_file):
        cm = self.csr_map

        def sanitize_field(name):
            s = name.replace("-", "_").replace(".", "_").replace(" ", "_").upper()
            return ("_" + s) if s and s[0].isdigit() else s

        fields_rows = []
        legal_values = []
        csr_rows = []
        for name in self.csr_names:
            csr = cm.csr_property_dict[name]
            field_first = len(fields_rows)
            for fname, field in csr.field.items():
                legal_first = len(legal_values)
                legal_str = str(field.legal_value or "").strip()
                vals = []
                for tok in legal_str.split(","):
                    tok = tok.strip()
                    if not tok:
                        continue
                    if tok.lower().startswith("0x"):
                        tok = tok[2:]
                    try:
                        vals.append(int(tok, 16) & 0xFFFFFFFFFFFFFFFF)
                    except ValueError:
                        continue
                legal_values.extend(vals)
                reset = cm.resolve_param_reset_value(field.reset_val, name, fname) & 0xFFFFFFFFFFFFFFFF
                fields_rows.append({
                    "name": sanitize_field(fname),
                    "msb": field.range[0],
                    "lsb": field.range[1],
                    "mask": self._field_mask(csr, field),
                    "reset": reset,
                    "legal_first": legal_first,
                    "legal_count": len(vals),
                    "sw_type": field.sw_type,
                })
            pol = self.policy_by_csr[name]
            csr_reset_str = csr.concat_reset_val(cm)
            csr_reset = int(str(csr_reset_str)[2:] or "0", 16) & 0xFFFFFFFFFFFFFFFF
            csr_rows.append({
                "name": name.lower(),
                "address": self._addr(csr),
                "size": csr.size,
                "reset": csr_reset,
                "write_mask": self.spec_write_mask(csr),
                "alias_of": self.csr_index.get(csr.alias_of, -1) if csr.alias_of else -1,
                "policy": pol,
                "field_first": field_first,
                "field_count": len(fields_rows) - field_first,
            })

        masked_targets = []
        cond_rows = []
        for cond in self.conditions:
            first = len(masked_targets)
            for target, mask in cond["targets"]:
                masked_targets.append((self.csr_index[target], mask))
            cond_rows.append({
                "name": cond["name"],
                "gate_csr": self.csr_index[cond["gate_csr"]],
                "gate_mask": cond["gate_mask"],
                "view_only": cond["view_only"],
                "target_first": first,
                "target_count": len(masked_targets) - first,
            })

        b = lambda v: "true" if v else "false"
        with open(output_file, "w") as f:
            w = f.write
            w("#pragma once\n")
            w("// Generated by csr_param_gen.py — CSRAL tables from the CSR spec + csral policy YAML.\n")
            w("#include <array>\n#include <cstdint>\n#include <string_view>\n\n")
            w("namespace CSRAL {\n\n")
            w("enum class on_mismatch_t : std::uint8_t { error = 0, skip = 1, resynch_rd = 2 };\n")
            w("enum class compare_mask_source_t : std::uint8_t { whisper = 0, spec = 1, both_warn = 2 };\n")
            w("enum class reset_check_t : std::uint8_t { error = 0, warn = 1, off = 2 };\n\n")
            w(f"inline constexpr compare_mask_source_t kCompareMaskSource = compare_mask_source_t::{self.config.compare_mask_source};\n")
            w(f"inline constexpr reset_check_t kResetCheck = reset_check_t::{self.config.reset_check};\n\n")
            w("struct policy_t {\n  bool check;\n  on_mismatch_t on_mismatch;\n  bool volatile_csr;\n  bool interrupt_class;\n  bool may_not_exist;\n  bool check_reset;\n};\n\n")
            w("struct field_t {\n  std::string_view name;\n  std::uint8_t msb;\n  std::uint8_t lsb;\n  std::uint64_t mask;\n  std::uint64_t reset;  // field-value, unshifted\n  std::uint16_t legal_first;\n  std::uint16_t legal_count;\n  std::string_view sw_type;\n};\n\n")
            w("struct csr_t {\n  std::string_view name;\n  std::uint16_t address;\n  std::uint16_t size;\n  std::uint64_t reset;  // concatenated CSR reset\n  std::uint64_t spec_write_mask;\n  std::int16_t alias_of;   // index into kCsrs, -1 = none\n  std::int16_t exists_if;  // index into kConditions gating this CSR's existence, -1 = always\n  policy_t policy;\n  std::uint16_t field_first;\n  std::uint16_t field_count;\n};\n\n")
            w("struct field_alias_t {\n  std::uint16_t csr;\n  std::uint8_t msb;\n  std::uint8_t lsb;\n  std::uint16_t alias_csr;\n  std::uint8_t alias_msb;\n  std::uint8_t alias_lsb;\n};\n\n")
            w("struct masked_target_t {\n  std::uint16_t csr;\n  std::uint64_t mask;\n};\n\n")
            w("struct condition_t {\n  std::string_view name;  // \"<gate_csr>.<gate_field>\", the +csral_save_restore token\n  std::uint16_t gate_csr;  // index into kCsrs\n  std::uint64_t gate_mask;\n  bool view_only;  // every target is an alias view: mask-only, never save/restore\n  std::uint16_t target_first;\n  std::uint16_t target_count;\n};\n\n")

            w(f"inline constexpr std::size_t kNumCsrs = {len(csr_rows)};\n")
            w(f"inline constexpr std::size_t kNumFields = {len(fields_rows)};\n\n")

            w(f"inline constexpr std::array<std::uint64_t, {max(len(legal_values), 1)}> kLegalValues = {{{{\n")
            for v in (legal_values or [0]):
                w(f"    0x{v:016X}ULL,\n")
            w("}};\n\n")

            w(f"inline constexpr std::array<field_t, {max(len(fields_rows), 1)}> kFields = {{{{\n")
            for r in (fields_rows or [{"name": "", "msb": 0, "lsb": 0, "mask": 0, "reset": 0, "legal_first": 0, "legal_count": 0, "sw_type": ""}]):
                w(f"    {{\"{r['name']}\", {r['msb']}, {r['lsb']}, 0x{r['mask']:016X}ULL, 0x{r['reset']:016X}ULL, {r['legal_first']}, {r['legal_count']}, \"{r['sw_type']}\"}},\n")
            w("}};\n\n")

            w(f"inline constexpr std::array<csr_t, {max(len(csr_rows), 1)}> kCsrs = {{{{\n")
            for r in (csr_rows or []):
                p = r["policy"]
                w(f"    {{\"{r['name']}\", 0x{r['address']:03X}, {r['size']}, 0x{r['reset']:016X}ULL, 0x{r['write_mask']:016X}ULL, {r['alias_of']}, {p['exists_if_index']}, "
                  f"{{{b(p['check'])}, on_mismatch_t::{p['on_mismatch']}, {b(p['volatile'])}, {b(p['interrupt'])}, {b(p['may_not_exist'])}, {b(p['check_reset'])}}}, "
                  f"{r['field_first']}, {r['field_count']}}},\n")
            w("}};\n\n")

            w(f"inline constexpr std::array<field_alias_t, {max(len(self.field_alias_rows), 1)}> kFieldAliases = {{{{\n")
            rows = [(self.csr_index[c], m, l, self.csr_index[a], am, al) for (c, m, l, a, am, al) in self.field_alias_rows] or [(0, 0, 0, 0, 0, 0)]
            for (c, m, l, a, am, al) in rows:
                w(f"    {{{c}, {m}, {l}, {a}, {am}, {al}}},\n")
            w("}};\n")
            w(f"inline constexpr std::size_t kNumFieldAliases = {len(self.field_alias_rows)};\n\n")

            w(f"inline constexpr std::array<masked_target_t, {max(len(masked_targets), 1)}> kMaskedTargets = {{{{\n")
            for (ci, mask) in (masked_targets or [(0, 0)]):
                w(f"    {{{ci}, 0x{mask:016X}ULL}},\n")
            w("}};\n\n")

            w(f"inline constexpr std::array<condition_t, {max(len(cond_rows), 1)}> kConditions = {{{{\n")
            for r in (cond_rows or [{"name": "", "gate_csr": 0, "gate_mask": 0, "view_only": False, "target_first": 0, "target_count": 0}]):
                w(f"    {{\"{r['name']}\", {r['gate_csr']}, 0x{r['gate_mask']:016X}ULL, {b(r['view_only'])}, {r['target_first']}, {r['target_count']}}},\n")
            w("}};\n")
            w(f"inline constexpr std::size_t kNumConditions = {len(cond_rows)};\n\n")

            w("// Address of a CSR with no direct CSR-bus address (indirect-only,\n")
            w("// e.g. behind miselect/mireg selectors).\n")
            w(f"inline constexpr std::uint16_t kNoDirectAddress = 0x{self.NO_DIRECT_ADDRESS:04X};\n\n")
            w("inline constexpr std::array<std::int16_t, 4096> kAddrIndex = []() {\n")
            w("  std::array<std::int16_t, 4096> a{};\n  for (auto& e : a)\n    e = -1;\n")
            for i, r in enumerate(csr_rows):
                if r["address"] != self.NO_DIRECT_ADDRESS:
                    w(f"  a[0x{r['address']:03X}] = {i};\n")
            w("  return a;\n}();\n\n")

            w("constexpr const csr_t* find_by_address(std::uint64_t addr) {\n")
            w("  if ((addr & ~0xFFFULL) != 0)\n    return nullptr;\n")
            w("  auto i = kAddrIndex[addr];\n")
            w("  return i < 0 ? nullptr : &kCsrs[static_cast<std::size_t>(i)];\n}\n\n")
            w("inline const csr_t* find_by_name(std::string_view name) {\n")
            w("  for (const auto& c : kCsrs) {\n    if (c.name == name)\n      return &c;\n  }\n  return nullptr;\n}\n\n")
            w("constexpr std::size_t index_of(const csr_t& c) {\n  return static_cast<std::size_t>(&c - kCsrs.data());\n}\n\n")
            w("constexpr std::uint64_t field_extract(const field_t& f, std::uint64_t csr_val) {\n  return (csr_val & f.mask) >> f.lsb;\n}\n\n")
            w("constexpr std::uint64_t field_insert(const field_t& f, std::uint64_t csr_val, std::uint64_t field_val) {\n  return (csr_val & ~f.mask) | ((field_val << f.lsb) & f.mask);\n}\n\n")
            w("} // namespace CSRAL\n")

        print(f"Generated CSRAL tables header: {output_file}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--csr_spec", type=str, help="Path to CSR specification YAML file")
    parser.add_argument("--csr_map_hpp", type=str, default="csr_map.hpp", help="Output C++ header file")
    parser.add_argument("--csr_map_sv", type=str, default="csr_map_pkg.sv", help="Output SystemVerilog defines file")
    parser.add_argument("--project_override", type=str, default=None, help="Path to YAML file with project parameter overrides")
    parser.add_argument("--csral_defaults", type=str, default=None, help="Path to the csral defaults YAML (policies/conditions config)")
    parser.add_argument("--csral_tables_hpp", type=str, default=None, help="Output CSRAL tables C++ header (requires --csral_defaults)")
    args = parser.parse_args()

    if not args.csr_spec:
        print("Error: --csr_spec argument is required")
        exit(1)
    csr_map = CsrMap(args.csr_spec, args.project_override)
    csr_map.generate_hpp_file(args.csr_map_hpp)
    csr_map.generate_sv_file(args.csr_map_sv)
    if args.csral_tables_hpp:
        if not args.csral_defaults:
            print("Error: --csral_tables_hpp requires --csral_defaults")
            exit(1)
        config = CsralConfig(args.csral_defaults, csr_map.override_data)
        model = CsralModel(csr_map, config)
        model.generate_tables_hpp(args.csral_tables_hpp)








