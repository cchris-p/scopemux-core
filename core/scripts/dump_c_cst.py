#!/usr/bin/env python3
"""
CST Dump Utility (Python)
------------------------

This script parses a C file and prints its Concrete Syntax Tree (CST) as JSON using the ScopeMux Python bindings.

Usage:
    python3 dump_c_cst.py <path-to-c-file>

Requires:
    - scopemux_core Python module (built via build_all_and_pybind.sh)
"""
import sys
import json

try:
    import scopemux_core
except ImportError:
    print(
        "ERROR: Could not import scopemux_core. Did you run build_all_and_pybind.sh?",
        file=sys.stderr,
    )
    sys.exit(1)


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 dump_c_cst.py <path-to-c-file>", file=sys.stderr)
        sys.exit(2)
    filename = sys.argv[1]
    cst = scopemux_core.parse_c_file_to_cst(filename)
    if cst is None:
        print(f"ERROR: Failed to parse CST for {filename}", file=sys.stderr)
        sys.exit(3)
    print(json.dumps(cst, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
