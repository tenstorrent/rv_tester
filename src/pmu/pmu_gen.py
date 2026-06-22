from typing import List, Dict, Any
import argparse
import csv
import io

pmcounter_dpi_width = 24

def read_csv_rows(data_str: str) -> List[Dict[Any, Any]]:
    """Read CSV rows, normalizing the header and dropping unnamed rows."""
    reader = csv.DictReader(io.StringIO(data_str))
    reader.fieldnames = [fn.lstrip('\ufeff').strip() for fn in (reader.fieldnames or [])]
    return [row for row in reader if row.get("Name")]

def make_event(name: str, description: str = "", event_id: Any = -1, core: bool = False) -> Dict[Any, Any]:
    """Build an event dict. 'description' is optional. Core events carry the
    extra multi-hot / multi-dimensional encoding fields."""
    event = {
        "name": name,
        "description": description,
        "event_id": event_id,
    }
    if core:
        event["multi_hot_encoding"] = "No"
        event["multi_D_filter"] = "No"
    return event

def parse_pmc_csv(data_str: str, core: bool, synthetic: List[Dict[Any, Any]]) -> List[Dict[Any, Any]]:
    """Parse PMC CSV data into a list of event dictionaries, appending the
    provided synthetic events. The 'Description' column is optional."""
    events = []
    for row in read_csv_rows(data_str):
        event = make_event(row["Name"], row.get("Description", ""), row["Event ID"], core=core)
        if core:
            event["multi_hot_encoding"] = row.get("Multi-hot Encoding", "No")
            event["multi_D_filter"] = row.get("Multi-dimensional Encoding", "No")
        events.append(event)
    events.extend(synthetic)
    return events

def parse_core_csv(data_str: str) -> List[Dict[Any, Any]]:
    """Parse core PMC CSV data into a list of event dictionaries."""
    synthetic = [
        make_event("branch_instructions", "Sum of retired branches", core=True),
        make_event("tb_cycles", "Event for each TB cycle", core=True),
    ]
    return parse_pmc_csv(data_str, core=True, synthetic=synthetic)

def parse_sc_csv(data_str: str) -> List[Dict[Any, Any]]:
    """Parse shared cache PMC CSV data into a list of event dictionaries."""
    synthetic = [
        make_event("sc_tb_cycles", "Event for each TB cycle"),
    ]
    return parse_pmc_csv(data_str, core=False, synthetic=synthetic)

IGNORE_NAMES = ["cpu_cycles", "instructions", "branch_instructions", "tb_cycles", "sc_tb_cycles"]


def write_cpp_group(f, events, enum_name, count_name, to_vector_name,
                     packet_ns, packet_struct, counter_array, to_string_name):
    """Write one counter group (enum + to_vector + reflection map) into the
    combined C++ header."""
    tab = "      "
    f.write("typedef enum : size_t {\n")
    for event in events:
        f.write(f"      //{event['description']}\n")
        f.write(f"      {event['name'].upper()},\n")
    f.write(f"    {count_name}\n    }} {enum_name};\n\n")

    f.write(f"    void {to_vector_name}(const rv_tester_transactions::{packet_ns}::{packet_struct}<>& pmcounters){{\n\n")
    f.write(f"      const uint64_t casting_size_term = uint64_t(1) << {pmcounter_dpi_width};\n\n")
    for event in events:
        name = event["name"]
        idx = f"{enum_name}::{name.upper()}"
        f.write(f"{tab}{counter_array}[{idx}] = {counter_array}[{idx}] + ((pmcounters.{name} - ({counter_array}[{idx}] % casting_size_term)) % casting_size_term);\n")
    f.write("    }\n\n")

    f.write(f"    const std::map<{enum_name}, std::string_view> {to_string_name} = {{\n")
    for event in events:
        name = event["name"]
        f.write(f'{tab}{{{name.upper()},"{name}"}},\n')
    f.write("    };\n")


def write_cpp_event_maps(f, core_events):
    """Write the core-only event_map and filtered_event_map."""
    tab = "      "
    f.write("\n\n    const std::unordered_map<uint64_t, size_t> event_map = {\n")
    for event in core_events:
        f.write(f'{tab}{{{event["event_id"]},counter_core::{event["name"].upper()}}},\n')
    f.write("    };")

    f.write("\n\n    const std::unordered_map<uint64_t, std::unordered_map<uint16_t, size_t>> filtered_event_map = {\n")
    filter_event_dict = {}
    for event in core_events:
        if event["multi_hot_encoding"] == "Yes" and event["multi_D_filter"] == "No":
            parent = event["event_id"][0:6]
            child = event["event_id"][6:10]
            filter_event_dict.setdefault(parent, {})[child] = event["name"]
    if '0x1462' in filter_event_dict: filter_event_dict['0x1862'] = filter_event_dict.pop('0x1462') #op_complete filtered event granularity should be 16
    if '0x1440' in filter_event_dict: filter_event_dict['0x1840'] = filter_event_dict.pop('0x1440') #op_issued filtered event granularity should be 16
    for parent, child_dict in filter_event_dict.items():
        filter_map_str = f"{tab}{{{parent}," + "{"
        for child, event_name in child_dict.items():
            bit_position = "0x" + child if child != '0' else None
            filter_map_str += f'\n{tab}{tab}{{{bit_position},counter_core::{event_name.upper()}}},'
        filter_map_str += "\n" + f'{tab}{tab}' + "}\n" + f'{tab}' + "},\n"
        f.write(filter_map_str)
    f.write("    };")


def create_cpp_frag(core_events, sc_events, path="gen_events.hpp"):
    """Generate the combined C++ events header (core + sc)."""
    guard = path.upper().replace('.', '_').replace('/', '_').replace('-', '_')
    with open(path, "w") as f:
        f.write(f"#ifndef {guard}\n#define {guard}\n\n")
        write_cpp_group(f, core_events, "counter_core", "COUNT_CORE", "core_to_vector",
                         "pmu_core", "pmcounters_core", "counters_core", "core_to_string")
        write_cpp_event_maps(f, core_events)
        f.write("\n\n")
        write_cpp_group(f, sc_events, "counter_sc", "COUNT_SC", "sc_to_vector",
                         "pmu_sc", "pmcounters_sc", "counters_sc", "sc_to_string")
        f.write(f"\n\n#endif // {guard}\n")


def build_events_sv(core_events, sc_events):
    """Build the auto-generated per-event counter assigns (core + sc)."""
    tab = "    "
    out = ""
    for event in core_events:
        name = event["name"]
        if name in IGNORE_NAMES:
            continue
        out += f"assign pmcounters_cores[0].data.{name} = {pmcounter_dpi_width}'(pmcounter[{name.upper()}]);\n"

    out += "\ngenerate\n    if (SC_PMCI_ENABLED == 1) begin\n"
    for event in sc_events:
        name = event["name"]
        if name in IGNORE_NAMES:
            continue
        out += f"{tab * 2}assign pmcounters_scs[0].data.{name} = {pmcounter_dpi_width}'(sc_pmci[{name.upper()}]);\n"
    out += "    end\nendgenerate\n"
    return out

def create_sv_frag(core_events, sc_events, path="gen_events.svh"):
    """Generate the combined SystemVerilog events fragment (core + sc)."""
    with open(path, 'w') as f:
        f.write(build_events_sv(core_events, sc_events))


def create_pmu_pkg(core_events, sc_events, path="gen_pmu_pkg.sv"):
    """Generate the combined pmu_pkg package (core + sc types)."""
    tab = "    "
    f = open(path, 'w')
    f.write("package pmu_pkg;\n\n")
    f.write(f"{tab}// --------------------------------------\n")
    f.write(f"{tab}// PMCI - Performance Monitoring Counters\n")
    f.write(f"{tab}// --------------------------------------\n")
    f.write(f"{tab}typedef enum {{\n")
    for event in core_events:
        f.write(f"{tab}{tab}{event['name'].upper()},\n")
    f.write(f"{tab}{tab}EVENT_COUNT\n")
    f.write(f"{tab}}} pmc_event_t;\n\n")
    f.write(f"{tab}typedef logic [3:0] pmc_counter_t;\n")
    f.write(f"{tab}typedef pmc_counter_t [EVENT_COUNT-1:0] pmci_t;\n\n")
    f.write(f"{tab}typedef enum {{\n")
    for i in range(3, 11):
        sep = ",\n" if i < 10 else "\n"
        f.write(f"{tab}{tab}HPMCOUNTER{i}{sep}")
    f.write(f"{tab}}} hpm_num_t;\n\n")
    f.write(f"{tab}typedef logic [63:0] hpm_counter_t;\n")
    f.write(f"{tab}typedef hpm_counter_t [7:0] hpmi_t;\n\n")
    f.write(f"{tab}// --------------------------------------\n")
    f.write(f"{tab}// Shared-cache PMCI\n")
    f.write(f"{tab}// --------------------------------------\n")
    f.write(f"{tab}parameter SC_COUNTER_HI = 32;\n\n")
    f.write(f"{tab}typedef enum {{\n")
    for event in sc_events:
        f.write(f"{tab}{tab}{event['name'].upper()},\n")
    f.write(f"{tab}{tab}SC_EVENT_COUNT\n")
    f.write(f"{tab}}} sc_pmc_event_t;\n\n")
    f.write(f"{tab}typedef logic [SC_COUNTER_HI:0] sc_pmc_counter_t;\n")
    f.write(f"{tab}typedef sc_pmc_counter_t [SC_EVENT_COUNT-1:0] sc_pmci_t;\n\n")
    f.write("endpackage\n")
    f.close()


def create_pmu_defines_frag(path="gen_pmu_defines.sv"):
    """Generate the PMU interface macros (RV_TESTER_PMCI_PORTS/VARS),
    mirroring RV_TESTER_AXI_PORTS/VARS. All signals are rv_tester outputs."""
    tab = "    "
    f = open(path, "w")
    f.write("`define RV_TESTER_PMCI_PORTS(input, output, pkg) \\\n")
    f.write(f"{tab}output pmu_pkg::pmci_t    pmci    [pkg::NHARTS-1:0], \\\n")
    f.write(f"{tab}output pmu_pkg::hpmi_t    hpmi    [pkg::NHARTS-1:0], \\\n")
    f.write(f"{tab}output pmu_pkg::sc_pmci_t sc_pmci\n\n")
    f.write("`define RV_TESTER_PMCI_VARS(pkg) \\\n")
    f.write(f"{tab}pmu_pkg::pmci_t    pmci    [pkg::NHARTS-1:0]; \\\n")
    f.write(f"{tab}pmu_pkg::hpmi_t    hpmi    [pkg::NHARTS-1:0]; \\\n")
    f.write(f"{tab}pmu_pkg::sc_pmci_t sc_pmci;\n")
    f.close()


def write_yaml_anchor(f, anchor, events):
    tab = "    "
    f.write(f"_{anchor}: &{anchor}\n")
    for event in events:
        f.write(f"{tab}{event['name']}:\n")
        f.write(f"{tab * 2}width: {pmcounter_dpi_width}\n")


def create_yaml_frag(core_events, sc_events, path="gen_events.yaml"):
    """Generate the combined transactions YAML (core + sc anchors).

    Each anchor is merged into rv_tester_transactions.yml via YAML merge
    syntax (<<: *anchor); packet_gen ignores anchors (no 'num' field)."""
    f = open(path, 'w')
    write_yaml_anchor(f, "pmcounters_core_fields", core_events)
    write_yaml_anchor(f, "pmcounters_sc_fields", sc_events)
    f.close()


def create_monitor_frag(core_events, sc_events, path="gen_monitor.svh"):
    """Generate the combined PMU monitor fragment (core + sc)."""
    f = open(path, 'w')
    for events, prefix in ((core_events, ""), (sc_events, "sc_")):
        f.write("always_comb begin\n")
        for event in events:
            name = event["name"]
            f.write(f"    {prefix}pmci[{name.upper()}] = {name};\n")
        f.write("end\n")
    f.close()

def generate_pmu_sv_content(core_events, sc_events, pmu_template):
    """Inline the combined events fragment into the pmu.sv template."""
    return pmu_template.replace('`include "gen_events.svh"',
                                build_events_sv(core_events, sc_events))


def main():
    parser = argparse.ArgumentParser(description='Generate PMU parameter files from CSV specifications')
    parser.add_argument('--core_pmc_csv', required=True, help='Path to core PMC CSV file')
    parser.add_argument('--sc_pmc_csv', required=True, help='Path to shared cache PMC CSV file')
    parser.add_argument('--gen_events_hpp', required=True, help='Output path for combined events C++ header')
    parser.add_argument('--gen_events_sv', required=True, help='Output path for combined events SystemVerilog')
    parser.add_argument('--gen_pmu_pkg_sv', required=True, help='Output path for combined pmu_pkg SystemVerilog')
    parser.add_argument('--gen_pmu_defines_sv', required=True, help='Output path for PMU interface defines (RV_TESTER_PMCI_PORTS/VARS)')
    parser.add_argument('--gen_monitor_sv', required=True, help='Output path for combined monitor SystemVerilog')
    parser.add_argument('--gen_events_yaml', required=True, help='Output path for combined events YAML')
    parser.add_argument('--pmu_template_sv', required=False, help='Path to pmu.sv template file')
    parser.add_argument('--gen_pmu_sv', required=False, help='Output path for generated pmu.sv')
    args = parser.parse_args()

    with open(args.core_pmc_csv, 'r') as f:
        core_data_str = f.read()
    with open(args.sc_pmc_csv, 'r') as f:
        sc_data_str = f.read()

    core_data_dict = parse_core_csv(core_data_str)
    sc_data_dict = parse_sc_csv(sc_data_str)

    create_cpp_frag(core_data_dict, sc_data_dict, args.gen_events_hpp)
    create_sv_frag(core_data_dict, sc_data_dict, args.gen_events_sv)
    create_pmu_pkg(core_data_dict, sc_data_dict, args.gen_pmu_pkg_sv)
    create_pmu_defines_frag(args.gen_pmu_defines_sv)
    create_monitor_frag(core_data_dict, sc_data_dict, args.gen_monitor_sv)
    create_yaml_frag(core_data_dict, sc_data_dict, args.gen_events_yaml)

    if args.pmu_template_sv and args.gen_pmu_sv:
        with open(args.pmu_template_sv, 'r') as f:
            pmu_template = f.read()
        pmu_sv_content = generate_pmu_sv_content(core_data_dict, sc_data_dict, pmu_template)
        with open(args.gen_pmu_sv, 'w') as f:
            f.write(pmu_sv_content)

if __name__ == "__main__":
    main()
