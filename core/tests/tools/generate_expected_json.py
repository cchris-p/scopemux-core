#!/usr/bin/env python3
"""
Generate Expected JSON Output for ScopeMux Test Cases

This script parses source code files using ScopeMux's parser bindings and
generates JSON representations of their AST/CST structures. It can be used
to update expected output files when the structure of AST/CST nodes changes.

Usage:
    python generate_expected_json.py [options] <source_file_or_dir>

Options:
    --output-dir DIR    Directory to write output files (default: same as input)
    --mode {ast,cst,both}   Parse mode: ast, cst, or both (default: both)
    --update            Update existing .expected.json files instead of creating new ones
    --review            Print diff before updating (implies --update)
    --dry-run           Do not write files, just print what would be done
    --verbose           Print detailed information during processing
"""

import sys
import os
import gc
import signal

# Print debugging information about the Python environment
print("DEBUG: Python sys.path:")
for i, path in enumerate(sys.path):
    print(f"  [{i}] {path}")
print(f"DEBUG: PYTHONPATH = {os.environ.get('PYTHONPATH', '')}")
print(f"DEBUG: LD_LIBRARY_PATH = {os.environ.get('LD_LIBRARY_PATH', '')}")

# Set up Python path to find the core module - ensure build/core is first
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
core_build_path = os.path.join(project_root, "build/core")

# Ensure build/core is first in sys.path
if core_build_path in sys.path:
    sys.path.remove(core_build_path)
sys.path.insert(0, core_build_path)


# Define a dummy segfault handler that can be used in case the real one isn't available
def dummy_segfault_handler(signum, frame):
    print("Warning: Using dummy segfault handler")
    raise RuntimeError("Segmentation fault detected (dummy handler)")


# Register the dummy handler
signal.signal(signal.SIGSEGV, dummy_segfault_handler)

# Import the scopemux_core module
try:
    import scopemux_core

    print(f"Loaded scopemux_core from: {scopemux_core.__file__}")
except ImportError as e:
    print(f"ERROR: Failed to import scopemux_core: {e}")
    print("Check that the module is built and in the correct location.")
    sys.exit(1)

# Try to get the segfault handler from the module
try:
    segfault_handler = scopemux_core.register_segfault_handler
    print(f"Found segfault_handler in {scopemux_core.__file__}")
except AttributeError:
    print(
        "Warning: scopemux_core.register_segfault_handler not found, using dummy handler"
    )
    segfault_handler = lambda: None

# Rest of the imports
import json
import argparse
import glob
import difflib
from pathlib import Path


def main():
    print("Starting main()")
    parser = argparse.ArgumentParser(
        description="Generate expected JSON output for test cases"
    )
    parser.add_argument("source_path", help="Source file or directory to process")
    parser.add_argument(
        "--output-dir", help="Directory to write output files (default: same as input)"
    )
    parser.add_argument(
        "--mode",
        choices=["ast", "cst", "both"],
        default="both",
        help="Parse mode: ast, cst, or both (default: both)",
    )
    parser.add_argument(
        "--update", action="store_true", help="Update existing .expected.json files"
    )
    parser.add_argument(
        "--review",
        action="store_true",
        help="Print diff before updating (implies --update)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Do not write files, just print what would be done",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print detailed information during processing",
    )

    args = parser.parse_args()

    # If --review is specified, --update is implied
    if args.review:
        args.update = True

    # Register the segfault handler
    segfault_handler()

    # Process the source path
    source_path = os.path.abspath(args.source_path)
    if os.path.isdir(source_path):
        print(f"Processing directory: {args.source_path}")
        process_directory(source_path, args)
    else:
        print(f"Processing file: {args.source_path}")
        process_file(source_path, args)


def process_directory(directory, args):
    # Process all supported file types in the directory
    for ext in [".c", ".h", ".cpp", ".hpp", ".py", ".js", ".ts"]:
        for file_path in glob.glob(
            os.path.join(directory, f"**/*{ext}"), recursive=True
        ):
            process_file(file_path, args)


def process_file(file_path, args):
    # Determine the language based on file extension
    ext = os.path.splitext(file_path)[1].lower()
    language = None

    if ext in [".c", ".h"]:
        language = scopemux_core.Language.C
    elif ext in [".cpp", ".hpp"]:
        language = scopemux_core.Language.CPP
    elif ext == ".py":
        language = scopemux_core.Language.PYTHON
    elif ext == ".js":
        language = scopemux_core.Language.JAVASCRIPT
    elif ext == ".ts":
        language = scopemux_core.Language.TYPESCRIPT
    else:
        print(f"Unsupported file extension: {ext}")
        return

    # Read the file content
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Create a parser context
    ctx = scopemux_core.ParserContext()

    # Parse the file
    if args.mode in ["ast", "both"]:
        try:
            ast_result = ctx.parse_string(content, file_path, language)
            if ast_result:
                ast_json = ctx.get_ast_json()
                output_path = get_output_path(file_path, "ast", args)
                write_or_update_json(ast_json, output_path, args)
            else:
                print(f"Failed to parse AST for {file_path}")
        except Exception as e:
            print(f"Error parsing AST for {file_path}: {e}")

    if args.mode in ["cst", "both"]:
        try:
            cst_result = ctx.parse_string(content, file_path, language)
            if cst_result:
                cst_json = ctx.get_cst_json()
                output_path = get_output_path(file_path, "cst", args)
                write_or_update_json(cst_json, output_path, args)
            else:
                print(f"Failed to parse CST for {file_path}")
        except Exception as e:
            print(f"Error parsing CST for {file_path}: {e}")

    # Clean up
    ctx.clear()


def get_output_path(file_path, mode, args):
    if args.output_dir:
        base_name = os.path.basename(file_path)
        return os.path.join(args.output_dir, f"{base_name}.{mode}.expected.json")
    else:
        return f"{file_path}.{mode}.expected.json"


def write_or_update_json(json_data, output_path, args):
    # Format the JSON with indentation for readability
    formatted_json = json.dumps(json_data, indent=2)

    if args.update and os.path.exists(output_path):
        if args.review:
            try:
                with open(output_path, "r", encoding="utf-8") as f:
                    existing_json = f.read()

                diff = list(
                    difflib.unified_diff(
                        existing_json.splitlines(),
                        formatted_json.splitlines(),
                        fromfile=f"a/{os.path.basename(output_path)}",
                        tofile=f"b/{os.path.basename(output_path)}",
                        lineterm="",
                    )
                )

                if diff:
                    print(f"\nDiff for {output_path}:")
                    for line in diff:
                        print(line)

                    if not args.dry_run:
                        print(f"Updating {output_path}")
                        with open(output_path, "w", encoding="utf-8") as f:
                            f.write(formatted_json)
                else:
                    print(f"No changes needed for {output_path}")
            except Exception as e:
                print(f"Error comparing files: {e}")
        else:
            if not args.dry_run:
                print(f"Updating {output_path}")
                with open(output_path, "w", encoding="utf-8") as f:
                    f.write(formatted_json)
            else:
                print(f"Would update {output_path} (dry run)")
    else:
        if not args.dry_run:
            print(f"Writing {output_path}")
            os.makedirs(os.path.dirname(output_path), exist_ok=True)
            with open(output_path, "w", encoding="utf-8") as f:
                f.write(formatted_json)
        else:
            print(f"Would write {output_path} (dry run)")


if __name__ == "__main__":
    main()
