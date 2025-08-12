import yaml
import logging 
import argparse

class Field:
    def __init__(self, name, width, msb, lsb, legal_value, reset_val, sw_type, perf_val):
        self.name = name
        self.width = width
        self.range = (msb, lsb)
        self.reset_val = reset_val
        self.legal_value = legal_value
        self.sw_type = sw_type
        self.perf_val = perf_val

class CSR:
    def __init__(self, name, address, size, alias_of=""):
        self.name = name
        self.address = address
        self.size = size
        self.field = {}
        self.alias_of = alias_of
        self.perf_val = "0x0"  # Initialize with default value

    def concat_perf_val(self, csr_map_instance=None):
        """Concatenate performance values from all fields in this CSR"""
        from bitstring import BitArray
        
        # Create a BitArray of the CSR size, initialized to 0
        concat_bitarray = BitArray(length=self.size)
        
        for field_name, field_property in self.field.items():
            # Use perf_val if available, otherwise fall back to reset_val
            value_to_use = None
            resolved_value = 0
            
            if hasattr(field_property, 'perf_val') and field_property.perf_val is not None and str(field_property.perf_val).strip():
                # Use the resolve_param_perf_value method if csr_map_instance is available
                if csr_map_instance:
                    resolved_value = csr_map_instance.resolve_param_perf_value(field_property.perf_val, self.name, field_name)
                else:
                    # Fallback to original logic if no csr_map_instance
                    val_str = str(field_property.perf_val).strip()
                    try:
                        if val_str.lower().startswith("0x"):
                            val_str = val_str[2:]
                        resolved_value = int(val_str, 16)
                    except (ValueError, TypeError):
                        resolved_value = 0
                value_to_use = resolved_value
            elif hasattr(field_property, 'reset_val') and field_property.reset_val:
                # Use the resolve_param_reset_value method if csr_map_instance is available
                if csr_map_instance:
                    resolved_value = csr_map_instance.resolve_param_reset_value(field_property.reset_val, self.name, field_name)
                else:
                    # Fallback to original logic if no csr_map_instance
                    val_str = str(field_property.reset_val).strip()
                    try:
                        if val_str.lower().startswith("0x"):
                            val_str = val_str[2:]
                        resolved_value = int(val_str, 16)
                    except (ValueError, TypeError):
                        resolved_value = 0
                value_to_use = resolved_value
            
            if value_to_use is not None:
                # Calculate start position (MSB position from the right)
                start_pos = self.size - 1 - field_property.range[0]  # range[0] is MSB
                
                try:
                    # Create BitArray for this field's value
                    field_bitarray = BitArray(length=field_property.width, uint=value_to_use)
                    # Overwrite the corresponding bits in the main BitArray
                    concat_bitarray.overwrite(field_bitarray, start_pos)
                except (ValueError, TypeError):
                    # If value is invalid, skip this field
                    continue
        
        # Convert to hex string
        hex_result = str(concat_bitarray)
        if hex_result.startswith('0x'):
            hex_result = hex_result[2:]
        
        # Remove leading zeros but keep at least one digit
        hex_result = hex_result.lstrip('0')
        if not hex_result:
            hex_result = '0'
        
        self.perf_val = '0x' + hex_result
        return self.perf_val

class CsrMap:
    def __init__(self, csr_spec):
        with open(csr_spec, "r") as c:
            description = yaml.safe_load(c)
        self.csr_property_dict = dict()
        # Shared parameter dictionary for both reset and performance values
        self.csr_params = {
            "CSR_CMCCFG_F_VLPRFSIZE_RESET_VALUE": 12,
            "CSR_CMCCFG_F_VECPRFSIZE_RESET_VALUE": 96,
            "CSR_CMCCFG_F_FPPRFSIZE_RESET_VALUE": 44,
            "CSR_CMCCFG_F_INTPRFSIZE_RESET_VALUE": 96,
            "CSR_CMCCFG_F_ROBSIZE_RESET_VALUE": 100,
            "CSR_CMCCFG1_F_DERRINTERRUPTNUMBER_RESET_VALUE": 23
            
        }
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
                width = field_data.get("FIELDS_WIDTH")
                field_range = field_data.get("FIELDS_RANGE")
                reset_val = field_data.get("RESET_VALUE", "0x0")
                legal_value = field_data.get("LEGAL_VALUE")
                
                # Parse MSB and LSB from FIELDS_RANGE (format like "63:0")
                if field_range and ":" in field_range:
                    msb, lsb = field_range.split(":")
                    msb, lsb = int(msb), int(lsb)
                else:
                    if not field_range:
                        logging.error(f"Range definition is not legal for field {field_name}")
                    else:
                        msb = lsb = int(field_range)
                
                width = msb - lsb + 1
                legal_value = field_data.get("LEGAL_VALUE", "0x0")
                sw_type = field_data.get("SW_TYPE", "WARL")
                if not sw_type:
                    sw_type = "WARL"
                perf_val = field_data.get("PERF_VALUE", None)
                
                field_obj = Field(field_name, width, msb, lsb, legal_value, reset_val, sw_type, perf_val)
                field_dict[field_name] = field_obj
            
            csr_obj = CSR(csr_name, csr_address, size, alias_of)
            csr_obj.field = field_dict
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
            """Convert address to 12-bit hex value (uint16_t)"""
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
            f.write("    std::uint16_t address;  // 12-bit hex address\n")
            f.write("    int size;\n")
            f.write("    csr_base* alias_of;  // Pointer to aliased CSR struct\n")
            f.write("    std::uint64_t perf_val;\n")
            f.write("    \n")
            f.write("    csr_base(const std::string& csr_name, std::uint16_t csr_address, int csr_size)\n")
            f.write("        : name(csr_name), address(csr_address), size(csr_size), alias_of(nullptr), perf_val(0) {}\n")
            f.write("    \n")
            f.write("    csr_base() = default;\n")
            f.write("};\n\n")
            
            for csr_name, csr in self.csr_property_dict.items():
                sanitized_csr_name = sanitize_name(csr_name).lower()
                struct_name = f"{sanitized_csr_name}_csr"
                
                f.write(f"struct {struct_name} : public csr_base {{\n")
                
                hex_address = parse_hex_address(csr.address)
                lowercase_csr_name = csr_name.lower()
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
                
                f.write(f"    {struct_name}() : csr_base(\"{lowercase_csr_name}\", 0x{hex_address:03X}, {csr.size}) {{\n")
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
            
            f.write("inline csr_base* find_csr_by_address(std::uint16_t address) {\n")
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
            sanitized = name.replace("-", "_").replace(".", "_").upper()
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
                    
                    # Field reset value - extract field value from full reset value
                    field_reset_val = 0
                    full_reset_val = self.resolve_param_reset_value(field.reset_val, csr_name, field_name)
                    if full_reset_val:
                        # Extract the field bits
                        field_reset_val = (full_reset_val >> lsb) & ((1 << width) - 1)
                    
                    f.write(f"parameter logic [{width-1}:0] {prefix}_RESET = {width}'h{field_reset_val:0{(width+3)//4}X};\n")
                    
                    # Field bit mask (for extracting field from full CSR value)
                    bit_mask = 0
                    for bit_pos in range(lsb, msb + 1):
                        if bit_pos < csr.size:
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


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--csr_spec", type=str, help="Path to CSR specification YAML file")
    parser.add_argument("--csr_map_hpp", type=str, default="csr_map.hpp", help="Output C++ header file")
    parser.add_argument("--csr_map_sv", type=str, default="csr_map_pkg.sv", help="Output SystemVerilog defines file")
    args = parser.parse_args()
    
    if not args.csr_spec:
        print("Error: --csr_spec argument is required")
        exit(1)
    csr_map = CsrMap(args.csr_spec)
    csr_map.generate_hpp_file(args.csr_map_hpp)
    csr_map.generate_sv_file(args.csr_map_sv)








