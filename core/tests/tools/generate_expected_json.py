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
    --verbose, -v       Verbose output
    --help, -h          Show this help message and exit

Examples:
    # Generate for a single C file (output will be hello_world.c.expected.json)
    python generate_expected_json.py core/tests/examples/c/basic_syntax/hello_world.c

    # Generate for all example files in a specific language directory
    python generate_expected_json.py core/tests/examples/c/

    # Generate for a specific directory with output files in a different location
    python generate_expected_json.py --output-dir core/tests/examples/c core/tests/examples/c/basic_syntax/

    # Generate for all test examples, updating existing files
    python generate_expected_json.py --update core/tests/examples/

    # Generate only AST output for Python files with review
    python generate_expected_json.py --mode ast --review core/tests/examples/python/
"""

import os
import sys
import json
import argparse
import difflib
from typing import Dict, List, Any, Optional, Tuple, Union

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__),
                '../../build/lib.linux-x86_64-3.10')))

print("sys.path:", sys.path)
print("build dir contents:", os.listdir(os.path.abspath(os.path.join(
    os.path.dirname(__file__), '../../build/lib.linux-x86_64-3.10'))))

try:
    import scopemux_core
except ImportError as e:
    print("Error: scopemux_core module not found.")
    print("Make sure the ScopeMux core Python bindings are installed.")
    print("ImportError details:", e)
    sys.exit(1)

# Constants
NODE_TYPE_MAPPING = {
    scopemux_core.NODE_FUNCTION: "FUNCTION",
    scopemux_core.NODE_METHOD: "METHOD",
    scopemux_core.NODE_CLASS: "CLASS",
    scopemux_core.NODE_STRUCT: "STRUCT",
    scopemux_core.NODE_ENUM: "ENUM",
    scopemux_core.NODE_INTERFACE: "INTERFACE",
    scopemux_core.NODE_NAMESPACE: "NAMESPACE",
    scopemux_core.NODE_MODULE: "MODULE",
    scopemux_core.NODE_COMMENT: "COMMENT",
    scopemux_core.NODE_DOCSTRING: "DOCSTRING",
    # Add any new node types here
}

# Language mapping
LANG_MAPPING = {
    scopemux_core.LANG_C: "C",
    scopemux_core.LANG_CPP: "C++",
    scopemux_core.LANG_PYTHON: "Python",
    scopemux_core.LANG_JAVASCRIPT: "JavaScript",
    scopemux_core.LANG_TYPESCRIPT: "TypeScript",
    # Add any new languages here
}


def ast_node_to_dict(node) -> Dict[str, Any]:
    """
    Convert an AST node to a dictionary representation (canonical schema).
    Always include all required fields, with null/empty where not present.
    """
    if not node:
        return {
            "type": None,
            "name": None,
            "qualified_name": None,
            "docstring": None,
            "signature": None,
            "return_type": None,
            "parameters": [],
            "path": None,
            "system": None,
            "range": None,
            "raw_content": None,
            "children": []
        }

    result = {
        "type": NODE_TYPE_MAPPING.get(node.get_type(), "UNKNOWN"),
        "name": node.get_name() or "",
        "qualified_name": node.get_qualified_name() or "",
        "docstring": node.get_docstring() if hasattr(node, "get_docstring") else None,
        "signature": node.get_signature() if hasattr(node, "get_signature") else None,
        "return_type": node.get_return_type() if hasattr(node, "get_return_type") else None,
        "parameters": [
            {
                "name": p.get_name() if hasattr(p, "get_name") else None,
                "type": p.get_type() if hasattr(p, "get_type") else None,
                "default": p.get_default() if hasattr(p, "get_default") else None
            } for p in node.get_parameters()
        ] if hasattr(node, "get_parameters") and node.get_parameters() else [],
        "path": node.get_path() if hasattr(node, "get_path") else None,
        "system": node.is_system() if hasattr(node, "is_system") else None,
        "range": None,
        "raw_content": node.get_raw_content() if hasattr(node, "get_raw_content") else None,
        "children": []
    }

    # Add source range information
    if hasattr(node, "get_range"):
        range_data = node.get_range()
        if range_data:
            result["range"] = {
                "start_line": getattr(range_data, "start_line", None),
                "start_column": getattr(range_data, "start_column", None),
                "end_line": getattr(range_data, "end_line", None),
                "end_column": getattr(range_data, "end_column", None),
            }

    # Recursively add children
    if hasattr(node, "get_children"):
        children = node.get_children()
        result["children"] = [ast_node_to_dict(child) for child in children] if children else []
    else:
        result["children"] = []

    # Always include all fields
    for key in ["docstring", "signature", "return_type", "parameters", "path", "system", "raw_content", "range", "children"]:
        if key not in result:
            result[key] = None if key != "parameters" and key != "children" else []

    return result


def cst_node_to_dict(node) -> Dict[str, Any]:
    """
    Convert a CST node to a dictionary representation (canonical schema).
    Always include all required fields, with null/empty where not present.
    """
    if not node:
        return {
            "type": None,
            "content": None,
            "range": None,
            "children": []
        }

    result = {
        "type": node.get_type() or "UNKNOWN",
        "content": node.get_content() or "",
        "range": None,
        "children": []
    }

    # Add range
    if hasattr(node, "get_range"):
        range_data = node.get_range()
        if range_data:
            result["range"] = {
                "start": {
                    "line": getattr(range_data, "start_line", None),
                    "column": getattr(range_data, "start_column", None),
                },
                "end": {
                    "line": getattr(range_data, "end_line", None),
                    "column": getattr(range_data, "end_column", None),
                },
            }

    # Recursively add children
    if hasattr(node, "get_children"):
        children = node.get_children()
        result["children"] = [cst_node_to_dict(child) for child in children] if children else []
    else:
        result["children"] = []

    # Always include all fields
    for key in ["type", "content", "range", "children"]:
        if key not in result:
            result[key] = None if key != "children" else []

    return result


def parse_file(
    file_path: str, mode: str = "both"
) -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """
    Parse a source file using ScopeMux parser and generate test AST/CST data.

    Args:
        file_path: Path to the source file
        mode: Parsing mode, one of "ast", "cst", "both"

    Returns:
        Tuple of (ast_dict, cst_dict) with extracted AST and CST data
    """
    try:
        parser = scopemux_core.ParserContext()
        detected_lang = scopemux_core.detect_language(file_path)
        lang_name = LANG_MAPPING.get(detected_lang, "Unknown")
        
        if not os.path.exists(file_path):
            raise RuntimeError(f"File not found: {file_path}")

        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()

        if hasattr(parser, "parse_string"):
            success = parser.parse_string(content, file_path, detected_lang)
        elif hasattr(parser, "parse_file"):
            success = parser.parse_file(file_path, detected_lang)
        else:
            raise RuntimeError("No suitable parse method found on ParserContext")

        if not success:
            get_err = getattr(parser, 'get_last_error', lambda: 'Unknown error')
            raise RuntimeError(f"Error parsing {file_path}: {get_err()}")

        ast_dict = None
        cst_dict = None

        if mode in ("ast", "both"):
            if not hasattr(parser, "get_ast_root"):
                raise RuntimeError("Parser bindings do not expose get_ast_root(). Cannot extract canonical AST.")
            ast_root = parser.get_ast_root()
            if not ast_root:
                raise RuntimeError(f"Parser did not return AST for {file_path}")
            ast_dict = ast_node_to_dict(ast_root)
        if mode in ("cst", "both"):
            if not hasattr(parser, "get_cst_root"):
                raise RuntimeError("Parser bindings do not expose get_cst_root(). Cannot extract canonical CST.")
            cst_root = parser.get_cst_root()
            if not cst_root:
                raise RuntimeError(f"Parser did not return CST for {file_path}")
            cst_dict = cst_node_to_dict(cst_root)
            if main_node and main_node["docstring"]:
                doc_start = main_node["range"]["start_line"] - 4
                doc_end = main_node["range"]["start_line"] - 1
                cst_children.append({
                    "type": "comment",
                    "content": main_node["docstring"],
                    "range": {
                        "start": {"line": doc_start, "column": 0},
                        "end": {"line": doc_end, "column": 3}
                    },
                    "children": []
                })
            # Function definition
            if main_node:
                cst_children.append({
                    "type": "function_definition",
                    "content": main_node["raw_content"],
                    "range": {
                        "start": {"line": main_node["range"]["start_line"], "column": 0},
                        "end": {"line": main_node["range"]["end_line"], "column": main_node["range"]["end_column"]}
                    },
                    "children": []
                })
            cst_dict = {
                "type": "translation_unit",
                "content": None,
                "range": {
                    "start": {"line": 0, "column": 0},
                    "end": {"line": len(lines), "column": 1}
                },
                "children": cst_children
            }

        print(f"Successfully processed {file_path}")
        return ast_dict, cst_dict

    except Exception as e:
        print(f"Exception in parse_file for {file_path}: {e}")
        import traceback
        traceback.print_exc()
        return None, None


def generate_combined_json(
    ast_dict: Optional[Dict[str, Any]], cst_dict: Optional[Dict[str, Any]], language: Optional[str] = None
) -> Dict[str, Any]:
    """
    Generate a combined JSON structure containing both AST and CST data for ScopeMux test cases.
    Always emits {language, ast, cst} per canonical schema.
    """
    return {
        "language": language or (ast_dict.get("language") if ast_dict and "language" in ast_dict else None),
        "ast": ast_dict,
        "cst": cst_dict,
    }

    # Insert AST if present
    if ast_dict:
        # Remove any non-AST fields (like 'language', 'file_path', etc.) if present
        ast_core = dict(ast_dict)
        ast_core.pop("language", None)
        ast_core.pop("file_path", None)
        ast_core.pop("parsed_successfully", None)
        ast_core.pop("content_length", None)
        if ast_core:  # Only add if non-empty
            result["ast"] = ast_core

    # Insert CST if present
    if cst_dict:
        cst_core = dict(cst_dict)
        cst_core.pop("language", None)
        cst_core.pop("file_path", None)
        cst_core.pop("parsed_successfully", None)
        cst_core.pop("content_length", None)
        if cst_core:
            result["cst"] = cst_core

    return result



def process_file(
    file_path: str,
    mode: str = "both",
    output_dir: Optional[str] = None,
    update: bool = False,
    review: bool = False,
    dry_run: bool = False,
    verbose: bool = False,
    root_dir: Optional[str] = None,
) -> bool:
    """
    Process a single source file and generate its expected JSON output.

    Args:
        file_path: Path to the source file
        mode: Parse mode ("ast", "cst", or "both")
        output_dir: Directory to write output file (default: same as input)
        update: Whether to update existing files
        review: Whether to show diffs before updating
        dry_run: Don't actually write files
        verbose: Print verbose output

    Returns:
        True if successful, False otherwise
    """
    if verbose:
        print(f"Processing {file_path}...")

    # Determine output path
    # Use the full filename (including extension) as the base for the output file name.
    base = os.path.basename(file_path)
    output_file_name = f"{base}.expected.json"

    if output_dir:
        # If root_dir is provided, preserve subdirectory structure
        if root_dir:
            rel_path = os.path.relpath(os.path.dirname(file_path), root_dir)
            output_subdir = os.path.join(output_dir, rel_path)
            os.makedirs(output_subdir, exist_ok=True)
            output_path = os.path.join(output_subdir, output_file_name)
        else:
            os.makedirs(output_dir, exist_ok=True)
            output_path = os.path.join(output_dir, output_file_name)
    else:
        output_path = os.path.join(
            os.path.dirname(file_path), output_file_name)

    # Check if output file exists
    file_exists = os.path.exists(output_path)
    if file_exists and not update:
        print(
            f"Output file {output_path} already exists. Use --update to overwrite.")
        return False

    # Parse the file
    ast_dict, cst_dict = parse_file(file_path, mode)
    if ast_dict is None and cst_dict is None:
        print(f"Failed to parse {file_path}")
        return False

    # Generate the combined JSON
    result_dict = generate_combined_json(ast_dict, cst_dict)

    # Convert to JSON string with indentation
    new_json = json.dumps(result_dict, indent=2, sort_keys=True)

    # Show diff if requested and file exists
    if review and file_exists:
        try:
            with open(output_path, "r") as f:
                old_json = f.read()

            old_lines = old_json.splitlines()
            new_lines = new_json.splitlines()

            diff = list(
                difflib.unified_diff(
                    old_lines,
                    new_lines,
                    fromfile=f"a/{output_path}",
                    tofile=f"b/{output_path}",
                    lineterm="",
                )
            )

            if diff:
                print("\nDiff for", output_path)
                print("\n".join(diff))
                print()

                # In an interactive context, you could ask for confirmation here
            else:
                print(f"No changes for {output_path}")
        except Exception as e:
            print(f"Error comparing files: {e}")

    # Write the output file
    if not dry_run:
        try:
            with open(output_path, "w") as f:
                f.write(new_json)
            if verbose:
                print(f"Wrote {output_path}")
            return True
        except Exception as e:
            print(f"Error writing {output_path}: {e}")
            return False
    else:
        if verbose:
            print(f"Would write {output_path} (dry run)")
        return True


def process_directory(dir_path: str, output_dir: Optional[str] = None, **kwargs) -> Tuple[int, int]:
    """
    Process all source files in a directory and its subdirectories.

    Args:
        dir_path: Path to the directory
        **kwargs: Additional arguments to pass to process_file

    Returns:
        Tuple of (success_count, failure_count)
    """
    success_count = 0
    failure_count = 0

    supported_extensions = [".c", ".cpp", ".h", ".hpp", ".py", ".js", ".ts"]

    for root, _, files in os.walk(dir_path):
        for file in files:
            # Skip non-source files and expected.json files
            if not any(
                file.endswith(ext) for ext in supported_extensions
            ) or file.endswith(".expected.json"):
                continue

            file_path = os.path.join(root, file)
            # Always pass root_dir as dir_path for correct relative path computation
            result = process_file(file_path, output_dir=output_dir, root_dir=dir_path, **kwargs)
            if result:
                success_count += 1
            else:
                print(
                    f"Error: Halting operation due to failure processing {file_path}")
                failure_count += 1
                return success_count, failure_count  # Immediately halt on failure

    return success_count, failure_count


def main():
    parser = argparse.ArgumentParser(
        description="Generate expected JSON output for ScopeMux test cases",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("\n\nUsage:")[
            0
        ],  # Use the module docstring for the epilog
    )
    parser.add_argument(
        "source_path", help="Source file or directory to process")
    parser.add_argument("--output-dir", help="Directory to write output files")
    parser.add_argument(
        "--mode",
        choices=["ast", "cst", "both"],
        default="both",
        help="Parse mode (default: both)",
    )
    parser.add_argument(
        "--update", action="store_true", help="Update existing .expected.json files"
    )
    parser.add_argument(
        "--review",
        action="store_true",
        help="Show diff before updating (implies --update)",
    )
    parser.add_argument(
        "--dry-run", action="store_true", help="Don't actually write files"
    )
    parser.add_argument("--verbose", "-v",
                        action="store_true", help="Verbose output")

    args = parser.parse_args()

    # If review is specified, update is implied
    if args.review:
        args.update = True

    # Process the source path
    if os.path.isdir(args.source_path):
        print(f"Processing directory: {args.source_path}")
        success, failure = process_directory(
            args.source_path,
            mode=args.mode,
            output_dir=args.output_dir,
            update=args.update,
            review=args.review,
            dry_run=args.dry_run,
            verbose=args.verbose,
        )
        print(
            f"Processed {success + failure} files: {success} succeeded, {failure} failed."
        )
    else:
        # Process a single file
        result = process_file(
            args.source_path,
            mode=args.mode,
            output_dir=args.output_dir,
            update=args.update,
            review=args.review,
            dry_run=args.dry_run,
            verbose=args.verbose,
        )
        print("Success" if result else "Failed")


if __name__ == "__main__":
    main()
