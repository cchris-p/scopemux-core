#!/usr/bin/env python3
"""
Dump the Concrete Syntax Tree (CST) for a C source file using the ScopeMux Python bindings.

Usage:
    python dump_c_cst.py <input_file.c>

This utility uses the scopemux_core Python extension, which must be built and importable.
"""
import sys
import json
import os

try:
    import scopemux_core
except ImportError:
    sys.stderr.write(
        "[ERROR] Could not import scopemux_core. Ensure the Python bindings are built and PYTHONPATH is set.\n"
    )
    sys.exit(2)


def main():
    if len(sys.argv) != 2:
        sys.stderr.write("Usage: python dump_c_cst.py <input_file.c>\n")
        sys.exit(1)
    input_path = sys.argv[1]
    if not os.path.isfile(input_path):
        sys.stderr.write(f"[ERROR] File not found: {input_path}\n")
        sys.exit(1)
    try:
        cst = scopemux_core.parse_c_file_to_cst(input_path)
        if cst is None:
            sys.stderr.write(f"[ERROR] CST parsing failed for file: {input_path}\n")
            sys.exit(1)
        # Dump CST as pretty JSON
        print(json.dumps(cst, indent=2, sort_keys=True))
    except Exception as e:
        sys.stderr.write(f"[ERROR] Exception during CST parsing: {e}\n")
        sys.exit(1)


if __name__ == "__main__":
    main()
