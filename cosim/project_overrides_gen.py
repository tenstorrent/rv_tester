#!/usr/bin/env python3

import argparse
import sys

import yaml


def _emit_enum(out, enum_name, entries):

    out.append(f"    typedef enum : size_t {{")
    for name, value in entries.items():
        out.append(f"        {name} = {value},")
    out.append(f"    }} {enum_name};")
    out.append("")

    out.append(f"    const std::unordered_map<size_t, std::string_view> {enum_name}_to_string = {{")
    for name in entries:
        out.append(f'        {{{name}, "{name}"}},')
    out.append(f"    }};")
    out.append("")


def generate(project_overrides_path):
    with open(project_overrides_path, "r") as f:
        data = yaml.safe_load(f) or {}

    exceptions = data.get("exceptions") or {}
    interrupts = data.get("interrupts") or {}

    out = []
    out.append(f"#pragma once")
    out.append(f"// AUTO-GENERATED from project_overrides.yaml by project_overrides_gen.py.")
    out.append(f"// DO NOT EDIT.")
    out.append(f"#include <cstddef>")
    out.append(f"#include <string_view>")
    out.append(f"#include <unordered_map>")
    out.append("")
    out.append(f"namespace {{")
    out.append("")

    if exceptions:
        _emit_enum(out, "custom_excp", exceptions)

    if interrupts:
        _emit_enum(out, "custom_intr", interrupts)

    out.append(f"}}  // namespace")
    out.append("")
    return "\n".join(out)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project_overrides", required=True,
                        help="Path to the project_overrides YAML file")
    parser.add_argument("--output", required=True,
                        help="Path to the generated C++ header to write")
    args = parser.parse_args(argv)

    header = generate(args.project_overrides)
    with open(args.output, "w") as f:
        f.write(header)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
