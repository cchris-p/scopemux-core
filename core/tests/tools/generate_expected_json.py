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
    # Generate for a single file
    python generate_expected_json.py core/test-cases/cpp/basic_syntax/hello_world.cpp

    # Generate for all test cases, updating existing files
    python generate_expected_json.py --update core/test-cases/

    # Generate for all C++ files with review
    python generate_expected_json.py --review core/test-cases/cpp/
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
    Convert an AST node to a dictionary representation.

    Args:
        node: AST node from scopemux_core

    Returns:
        Dict representing the node's properties
    """
    if not node:
        return {}

    # Basic node properties
    result = {
        "type": NODE_TYPE_MAPPING.get(node.get_type(), "UNKNOWN"),
        "name": node.get_name() or "",
        "qualified_name": node.get_qualified_name() or "",
    }

    # Optional properties - only include if they exist
    if node.get_signature():
        result["signature"] = node.get_signature()

    if node.get_docstring():
        result["docstring"] = node.get_docstring()

    if node.get_raw_content():
        result["raw_content"] = node.get_raw_content()

    # Add source range information
    if hasattr(node, "get_range"):
        range_data = node.get_range()
        if range_data:
            result["range"] = {
                "start_line": range_data.start.line,
                "start_column": range_data.start.column,
                "end_line": range_data.end.line,
                "end_column": range_data.end.column,
            }
    else:
        # Add empty range structure to maintain schema consistency
        result["range"] = {
            "start_line": 0,
            "start_column": 0,
            "end_line": 0,
            "end_column": 0,
        }

    # Process children recursively
    children = []
    # Note: The Python bindings would need a get_children() method
    # This is a placeholder for when that method exists
    for child_node in getattr(node, "get_children", lambda: [])():
        children.append(ast_node_to_dict(child_node))

    if children:
        result["children"] = children

    # Add references
    references = []
    # Add references when available from bindings
    if hasattr(node, "get_references"):
        for ref_node in node.get_references():
            references.append(ast_node_to_dict(ref_node))

    if references:
        result["references"] = references

    return result


def cst_node_to_dict(node) -> Dict[str, Any]:
    """
    Convert a CST node to a dictionary representation.

    Args:
        node: CST node from scopemux_core

    Returns:
        Dict representing the node's properties
    """
    if not node:
        return {}

    # Basic node properties
    result = {
        "type": node.get_type() if hasattr(node, "get_type") else "UNKNOWN",
        "content": node.get_content() if hasattr(node, "get_content") else "",
    }

    # Add source range information
    if hasattr(node, "get_range"):
        range_data = node.get_range()
        if range_data:
            result["range"] = {
                "start_line": range_data.start.line,
                "start_column": range_data.start.column,
                "end_line": range_data.end.line,
                "end_column": range_data.end.column,
            }
    else:
        # Add empty range structure to maintain schema consistency
        result["range"] = {
            "start_line": 0,
            "start_column": 0,
            "end_line": 0,
            "end_column": 0,
        }

    # Process children recursively
    children = []
    # Note: The Python bindings would need a get_children() method
    # This is a placeholder for when that method exists
    for child_node in getattr(node, "get_children", lambda: [])():
        children.append(cst_node_to_dict(child_node))

    if children:
        result["children"] = children

    return result


def parse_file(
    file_path: str, mode: str = "both"
) -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """
    Parse a file and return its AST and/or CST as dictionaries.

    Args:
        file_path: Path to the file to parse
        mode: Parse mode ("ast", "cst", or "both")

    Returns:
        Tuple of (ast_dict, cst_dict). Either may be None depending on mode.
    """
    parser = scopemux_core.ParserContext()

    # Detect language from file extension
    detected_lang = scopemux_core.detect_language(file_path)
    lang_name = LANG_MAPPING.get(detected_lang, "Unknown")

    ast_dict = None
    cst_dict = None

    # Parse the file in AST mode
    if mode in ("ast", "both"):
        # Assuming the parser has a method to set the mode
        # If not available, we'll need to adjust this logic
        if hasattr(parser, "set_mode"):
            parser.set_mode(scopemux_core.PARSE_AST)

        # Pass the detected language to parse_file to avoid "Unsupported language" error
        success = parser.parse_file(file_path, language=detected_lang)
        if not success:
            print(
                f"Error parsing {file_path} in AST mode: {parser.get_last_error()}")
            return None, None

        # Assuming there's a method to get the AST root
        # This will need adjustment based on actual API
        ast_root = parser.get_ast_root() if hasattr(parser, "get_ast_root") else None

        if ast_root:
            ast_dict = {"language": lang_name,
                        "ast_root": ast_node_to_dict(ast_root)}

    # Parse the file in CST mode
    if mode in ("cst", "both"):
        # Create a new parser for CST mode to avoid state issues
        cst_parser = scopemux_core.ParserContext()

        if hasattr(cst_parser, "set_mode"):
            cst_parser.set_mode(scopemux_core.PARSE_CST)

        success = cst_parser.parse_file(file_path)
        if not success:
            print(
                f"Error parsing {file_path} in CST mode: {cst_parser.get_last_error()}"
            )
            return ast_dict, None

        # Assuming there's a method to get the CST root
        # This will need adjustment based on actual API
        cst_root = (
            cst_parser.get_cst_root() if hasattr(cst_parser, "get_cst_root") else None
        )

        if cst_root:
            cst_dict = {"language": lang_name,
                        "cst_root": cst_node_to_dict(cst_root)}

    return ast_dict, cst_dict


def generate_combined_json(
    ast_dict: Optional[Dict[str, Any]], cst_dict: Optional[Dict[str, Any]]
) -> Dict[str, Any]:
    """
    Generate a combined JSON structure containing both AST and CST data.

    Args:
        ast_dict: The AST dictionary (may be None)
        cst_dict: The CST dictionary (may be None)

    Returns:
        A dict containing the combined structure
    """
    result = {}

    # Add language info
    if ast_dict:
        result["language"] = ast_dict.get("language", "Unknown")
    elif cst_dict:
        result["language"] = cst_dict.get("language", "Unknown")

    # Add AST data if available, using "ast" as the key
    if ast_dict and "ast_root" in ast_dict:
        result["ast"] = ast_dict["ast_root"]

    # Add CST data if available (optional, not part of strict AST schema)
    if cst_dict and "cst_root" in cst_dict:
        result["cst_root"] = cst_dict["cst_root"]

    return result


def process_file(
    file_path: str,
    mode: str = "both",
    output_dir: Optional[str] = None,
    update: bool = False,
    review: bool = False,
    dry_run: bool = False,
    verbose: bool = False,
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
    base_name = os.path.basename(file_path)
    output_file_name = f"{base_name}.expected.json"

    if output_dir:
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


def process_directory(dir_path: str, **kwargs) -> Tuple[int, int]:
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
            result = process_file(file_path, **kwargs)
            if result:
                success_count += 1
            else:
                print(f"Error: Halting operation due to failure processing {file_path}")
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
