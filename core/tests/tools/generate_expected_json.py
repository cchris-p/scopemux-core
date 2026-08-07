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

from pathlib import Path
import difflib
import glob
import argparse
import json
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
project_root = os.path.abspath(os.path.join(
    os.path.dirname(__file__), "../../.."))
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
    def segfault_handler(): return None

# Rest of the imports


def main():
    print("Starting main()")
    parser = argparse.ArgumentParser(
        description="Generate expected JSON output for test cases"
    )
    parser.add_argument(
        "source_path", help="Source file or directory to process")
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


def serialize_ast_node_to_dict(ast_node):
    """Convert an ASTNodeObject to a dictionary suitable for JSON serialization."""
    if not ast_node:
        return None

    try:
        node_dict = {
            "type": ast_node.get_type() if hasattr(ast_node, 'get_type') else None,
            "name": ast_node.get_name() if hasattr(ast_node, 'get_name') else None,
            "qualified_name": ast_node.get_qualified_name() if hasattr(ast_node, 'get_qualified_name') else None,
            "signature": ast_node.get_signature() if hasattr(ast_node, 'get_signature') else None,
            "docstring": ast_node.get_docstring() if hasattr(ast_node, 'get_docstring') else None,
        }

        # Remove None values to keep the output clean
        return {k: v for k, v in node_dict.items() if v is not None}
    except Exception as e:
        print(f"Error serializing AST node: {e}")
        return None


def process_file(file_path, args):
    # Determine the language based on file extension
    ext = os.path.splitext(file_path)[1].lower()
    language = None

    if ext in [".c", ".h"]:
        language = scopemux_core.LANG_C
    elif ext in [".cpp", ".hpp"]:
        language = scopemux_core.LANG_CPP
    elif ext == ".py":
        language = scopemux_core.LANG_PYTHON
    elif ext == ".js":
        language = scopemux_core.LANG_JAVASCRIPT
    elif ext == ".ts":
        language = scopemux_core.LANG_TYPESCRIPT
    else:
        print(f"Unsupported file extension: {ext}")
        return

    # Read the file content
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Create a parser context
    ctx = scopemux_core.ParserContext()

    ast_json = None
    cst_json = None

    # Parse the file once
    try:
        parse_result = ctx.parse_string(content, file_path, language)
        if not parse_result:
            print(f"Failed to parse {file_path}")
            return
    except Exception as e:
        print(f"Error parsing {file_path}: {e}")
        return

    # Extract AST if requested
    if args.mode in ["ast", "both"]:
        try:
            ast_root = ctx.get_ast_root()
            if ast_root:
                ast_json = serialize_ast_node_to_dict(ast_root)
                print(
                    f"DEBUG: AST root extracted for {file_path}, type: {type(ast_root)}")
            else:
                print(f"No AST root found for {file_path}")
        except Exception as e:
            print(f"Error extracting AST for {file_path}: {e}")

    # Extract CST if requested
    if args.mode in ["cst", "both"]:
        try:
            if hasattr(ctx, "get_cst_root"):
                cst_root = ctx.get_cst_root()
                if cst_root:
                    cst_json = cst_root  # get_cst_root already returns a dictionary
                    print(
                        f"DEBUG: CST root extracted for {file_path}, type: {type(cst_root)}")
                else:
                    print(f"No CST root found for {file_path}")
            else:
                print(
                    "Warning: get_cst_root() not available in scopemux_core.ParserContext")
        except Exception as e:
            print(f"Error extracting CST for {file_path}: {e}")

    # Debug: print AST and CST output
    print(f"DEBUG: AST for {file_path}: {repr(ast_json)}")
    print(f"DEBUG: CST for {file_path}: {repr(cst_json)}")

    # Write combined .expected.json file if either AST or CST is present
    combined = {}
    if ast_json is not None:
        combined["ast"] = ast_json
    if cst_json is not None:
        combined["cst"] = cst_json

    # Add language field if available
    if hasattr(ctx, "language"):
        combined["language"] = ctx.language
    else:
        # Fallback: try to infer from language variable
        lang_map = {
            scopemux_core.LANG_C: "C",
            scopemux_core.LANG_CPP: "C++",
            scopemux_core.LANG_PYTHON: "Python",
            scopemux_core.LANG_JAVASCRIPT: "JavaScript",
            scopemux_core.LANG_TYPESCRIPT: "TypeScript",
        }
        combined["language"] = lang_map.get(language, "unknown")

    # Write to .expected.json file
    output_path = f"{file_path}.expected.json"
    write_or_update_json(combined, output_path, args)
    # Clean up
    # ctx.clear()  # Removed to avoid AttributeError


# get_output_path is no longer needed for combined output


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
