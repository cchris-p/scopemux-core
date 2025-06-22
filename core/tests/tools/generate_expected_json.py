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
    
    Handles both object-style CST nodes and dictionary-based CST nodes.
    Creates a deep copy to ensure no references to C-managed memory remain.
    """
    # Initialize result with default values
    result = {
        "type": "UNKNOWN",
        "content": "",
        "range": None,
        "children": []
    }
    
    if not node:
        return result
    
    try:
        # Check if we already have a dictionary object
        # (which is what our new C binding implementation returns)
        if isinstance(node, dict):
            # Handle type field - safely create a copy of simple data types
            if "type" in node:
                result["type"] = str(node["type"]) if node["type"] else "UNKNOWN"
            elif callable(getattr(node, "get_type", None)):
                result["type"] = str(node.get_type() or "UNKNOWN")
                
            # Handle content field - safely create a copy
            if "content" in node:
                result["content"] = str(node["content"]) if node["content"] else ""
            elif callable(getattr(node, "get_content", None)):
                result["content"] = str(node.get_content() or "")
                
            # Handle children - initialize empty list
            children = []
            # Either from dictionary children array
            if "children" in node and isinstance(node["children"], list):
                # Only get reference to children, we'll make deep copies below
                children = node["children"]
            # Or from method
            elif callable(getattr(node, "get_children", None)):
                children = node.get_children() or []
            
            # Handle range - create a completely new structure
            if "range" in node and node["range"]:
                range_data = node["range"]
                if isinstance(range_data, dict) and 'start' in range_data and 'end' in range_data:
                    start = range_data['start']
                    end = range_data['end']
                    
                    result["range"] = {
                        "start": {
                            "line": int(start.get('line', 0)),
                            "column": int(start.get('column', 0)),
                        },
                        "end": {
                            "line": int(end.get('line', 0)),
                            "column": int(end.get('column', 0)),
                        },
                    }
            elif callable(getattr(node, "get_range", None)):
                range_data = node.get_range()
                if range_data:
                    # Handle different range structures:
                    # 1. Dictionary with nested start/end dicts (from our CSTNode)
                    if isinstance(range_data, dict) and 'start' in range_data and 'end' in range_data:
                        start = range_data['start']
                        end = range_data['end']
                        
                        result["range"] = {
                            "start": {
                                "line": int(start.get('line', 0)),
                                "column": int(start.get('column', 0)),
                            },
                            "end": {
                                "line": int(end.get('line', 0)),
                                "column": int(end.get('column', 0)),
                            },
                        }
                    # 2. Object with start_line/end_line attributes (older style)
                    elif all(hasattr(range_data, attr) for attr in ['start_line', 'end_line', 'start_column', 'end_column']):
                        result["range"] = {
                            "start": {
                                "line": int(getattr(range_data, "start_line", 0)),
                                "column": int(getattr(range_data, "start_column", 0)),
                            },
                            "end": {
                                "line": int(getattr(range_data, "end_line", 0)),
                                "column": int(getattr(range_data, "end_column", 0)),
                            },
                        }
            
            # Recursively process children (this is critical to avoid segfaults)
            # Creating a new list and not keeping any references to original children
            child_list = []
            if children:
                for child in children:
                    if child is not None:  # Safety check
                        try:
                            child_dict = cst_node_to_dict(child) 
                            if child_dict:  # Only add non-empty children
                                child_list.append(child_dict)
                        except Exception as e:
                            print(f"Error processing dictionary-based child node: {e}")
                        
            # Store the fully processed children list
            result["children"] = child_list
            
        else:
            # Traditional object-based CST node - create a safe deep copy
            # Create basic fields with safe string conversions
            if hasattr(node, "get_type"):
                result["type"] = str(node.get_type() or "UNKNOWN")
                
            if hasattr(node, "get_content"):
                result["content"] = str(node.get_content() or "")

            # Add range with explicit type conversions
            if hasattr(node, "get_range"):
                range_data = node.get_range()
                if range_data:
                    # Handle different range structures:
                    # 1. Dictionary with nested start/end dicts (from our CSTNode)
                    if isinstance(range_data, dict) and 'start' in range_data and 'end' in range_data:
                        start = range_data['start']
                        end = range_data['end']
                        
                        result["range"] = {
                            "start": {
                                "line": int(start.get('line', 0)),
                                "column": int(start.get('column', 0)),
                            },
                            "end": {
                                "line": int(end.get('line', 0)),
                                "column": int(end.get('column', 0)),
                            },
                        }
                    # 2. Object with start_line/end_line attributes (older style)
                    elif all(hasattr(range_data, attr) for attr in ['start_line', 'end_line', 'start_column', 'end_column']):
                        result["range"] = {
                            "start": {
                                "line": int(getattr(range_data, "start_line", 0)),
                                "column": int(getattr(range_data, "start_column", 0)),
                            },
                            "end": {
                                "line": int(getattr(range_data, "end_line", 0)),
                                "column": int(getattr(range_data, "end_column", 0)),
                            },
                        }

            # Process children with safety checks
            child_list = []
            if hasattr(node, "get_children"):
                children = node.get_children()
                if children:
                    for child in children:
                        if child is not None:  # Safety check
                            try:
                                child_dict = cst_node_to_dict(child)
                                if child_dict:  # Only add non-empty children
                                    child_list.append(child_dict)
                            except Exception as e:
                                print(f"Error processing object-based child node: {e}")
            
            result["children"] = child_list
    except Exception as e:
        print(f"Error in cst_node_to_dict: {e}")
        # On exception, we'll still return the default result dictionary
        
    return result


_processed_nodes = set()

def cleanup_cst_tree(root_node):
    """
    Creates a safe, JSON-serializable dictionary version of the CST tree.
    This is needed because the CST nodes may be tied to underlying C memory
    that will be freed when the ParserContext is cleaned up. Converting to
    a pure Python dictionary avoids segfaults when GC runs.
    
    This is a thin wrapper around cleanup_cst_node that clears the processed
    node tracking set before starting.
    """
    # Clear the processed nodes set to avoid issues with multiple calls
    _processed_nodes.clear()
    
    # Defensively handle root_node being None
    if root_node is None:
        print("Warning: root_node is None in cleanup_cst_tree")
        return {
            "type": "ROOT",
            "content": "",
            "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
            "children": []
        }
    
    # For dictionary nodes, use a safer copy approach instead of object modification
    if isinstance(root_node, dict):
        print("Using safe deep copy for dictionary-based CST node")
        # Make a deep copy using cst_node_to_dict which is safer
        return cst_node_to_dict(root_node)
    
    # For object-based nodes, use the cleanup_cst_node function
    print("Using cleanup_cst_node for object-based CST node")
    return cleanup_cst_node(root_node)


def cleanup_cst_node(node):
    """
    Helper function to recursively clean up CST nodes by creating a pure dictionary representation.
    Avoids processing the same node twice.
    
    Handles both dictionary-style nodes and object-style nodes.
    """
    # Check if already processed to avoid recursion issues
    if node is None:
        return {
            "type": "UNKNOWN",
            "content": "",
            "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
            "children": []
        }
        
    node_id = id(node)
    if node_id in _processed_nodes:
        # Already processed this node, avoid infinite recursion
        return {
            "type": "REFERENCE", 
            "content": f"Circular reference to node {node_id}",
            "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
            "children": []
        }
    
    _processed_nodes.add(node_id)
    
    try:
        # Check if we already have a dictionary
        if isinstance(node, dict):
            # For dictionary nodes, create a new dictionary with all required fields
            result = {
                "type": str(node.get("type", "UNKNOWN")),
                "content": str(node.get("content", "")),
                "children": []
            }
            
            # Copy range data if available, creating a proper deep copy
            if "range" in node and node["range"]:
                range_data = node["range"]
                if isinstance(range_data, dict) and "start" in range_data and "end" in range_data:
                    start = range_data["start"]
                    end = range_data["end"]
                    result["range"] = {
                        "start": {
                            "line": int(start.get("line", 0)) if isinstance(start, dict) else 0,
                            "column": int(start.get("column", 0)) if isinstance(start, dict) else 0
                        },
                        "end": {
                            "line": int(end.get("line", 0)) if isinstance(end, dict) else 0,
                            "column": int(end.get("column", 0)) if isinstance(end, dict) else 0
                        }
                    }
                else:
                    result["range"] = {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}}
            else:
                result["range"] = {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}}
                
            # Process children recursively, handling errors gracefully
            children = []
            if "children" in node and isinstance(node["children"], list):
                for child in node["children"]:
                    try:
                        # Skip None children
                        if child is None:
                            continue
                            
                        child_dict = cleanup_cst_node(child)
                        if child_dict:  # Only add non-empty children
                            children.append(child_dict)
                    except Exception as e:
                        print(f"Error processing child in cleanup_cst_node: {e}")
                        # Add an error node instead of failing
                        children.append({
                            "type": "ERROR",
                            "content": str(e),
                            "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
                            "children": []
                        })
            result["children"] = children
            
            # Remove any special method-style references from dictionary-based nodes
            # We now return dictionaries from C bindings, so this should not be needed
            # But we keep it for backward compatibility
        else:
            # For object-based nodes, extract values via methods with safety checks
            result = {}
            
            # Safely extract type
            try:
                if hasattr(node, "get_type") and callable(getattr(node, "get_type")):
                    result["type"] = str(node.get_type() or "UNKNOWN")
                else:
                    result["type"] = "UNKNOWN"
            except Exception as e:
                print(f"Error getting node type: {e}")
                result["type"] = "ERROR"
                
            # Safely extract content
            try:
                if hasattr(node, "get_content") and callable(getattr(node, "get_content")):
                    result["content"] = str(node.get_content() or "")
                else:
                    result["content"] = ""
            except Exception as e:
                print(f"Error getting node content: {e}")
                result["content"] = str(e)
            
            # Default empty range structure
            result["range"] = {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}}
            
            # Safely extract range data
            try:
                if hasattr(node, "get_range") and callable(getattr(node, "get_range")):
                    range_data = node.get_range()
                    if range_data:
                        if isinstance(range_data, dict) and "start" in range_data and "end" in range_data:
                            # Dictionary-style range
                            start = range_data["start"]
                            end = range_data["end"]
                            
                            if isinstance(start, dict) and isinstance(end, dict):
                                result["range"] = {
                                    "start": {
                                        "line": int(start.get("line", 0)), 
                                        "column": int(start.get("column", 0))
                                    },
                                    "end": {
                                        "line": int(end.get("line", 0)), 
                                        "column": int(end.get("column", 0))
                                    }
                                }
                        # Object-style range with attributes
                        elif all(hasattr(range_data, attr) for attr in ["start_line", "end_line", "start_column", "end_column"]):
                            result["range"] = {
                                "start": {
                                    "line": int(getattr(range_data, "start_line", 0)),
                                    "column": int(getattr(range_data, "start_column", 0))
                                },
                                "end": {
                                    "line": int(getattr(range_data, "end_line", 0)),
                                    "column": int(getattr(range_data, "end_column", 0))
                                }
                            }
            except Exception as e:
                print(f"Error getting range data: {e}")
                # Range already has default values

            # Process children with safety checks
            result["children"] = []
            try:
                if hasattr(node, "get_children") and callable(getattr(node, "get_children")):
                    children = node.get_children()
                    if children:
                        child_list = []
                        for child in children:
                            if child is None:
                                continue
                                
                            try:
                                child_dict = cleanup_cst_node(child)
                                if child_dict:  # Only add non-empty children
                                    child_list.append(child_dict)
                            except Exception as child_e:
                                print(f"Error processing child: {child_e}")
                                # Add a placeholder for the problematic child
                                child_list.append({
                                    "type": "ERROR",
                                    "content": str(child_e),
                                    "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
                                    "children": []
                                })
                        result["children"] = child_list
            except Exception as e:
                print(f"Error processing children: {e}")
    
        return result
    except Exception as e:
        print(f"Error in cleanup_cst_node: {e}")
        return {}


def parse_file(file_path: str, mode: str = "both") -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """
    Parse a source file using ScopeMux parser and generate test AST/CST data.

    Args:
        file_path: Path to the source file
        mode: Parsing mode, one of "ast", "cst", "both"

    Returns:
        Tuple of (ast_dict, cst_dict) with extracted AST and CST data
    """
    import gc  # Import garbage collection module for explicit cleanup
    
    try:
        parser = scopemux_core.ParserContext()
        detected_lang = scopemux_core.detect_language(file_path)
        lang_name = LANG_MAPPING.get(detected_lang, "Unknown")
        
        if not os.path.exists(file_path):
            raise RuntimeError(f"File not found: {file_path}")

        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
            lines = content.splitlines()

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

        # Process AST if needed
        if mode in ("ast", "both"):
            if not hasattr(parser, "get_ast_root"):
                raise RuntimeError("Parser bindings do not expose get_ast_root(). Cannot extract canonical AST.")
            ast_root = parser.get_ast_root()
            if not ast_root:
                raise RuntimeError(f"Parser did not return AST for {file_path}")
            ast_dict = ast_node_to_dict(ast_root)
            # Explicitly dereference the AST root to help garbage collection
            ast_root = None
            
        # Process CST if needed
        if mode in ("cst", "both"):
            try:
                if not hasattr(parser, "get_cst_root"):
                    raise RuntimeError("Parser bindings do not expose get_cst_root(). Cannot extract canonical CST.")
                    
                # Get the CST root from parser - this should now return a Python dictionary directly
                print("About to get CST root...")
                cst_root = parser.get_cst_root()
                print(f"Got CST root of type: {type(cst_root)}")
                
                if cst_root is None:
                    print(f"Warning: Parser returned None for CST for {file_path}")
                    cst_dict = {
                        "type": "ROOT", 
                        "content": "",
                        "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
                        "children": []
                    }
                elif isinstance(cst_root, dict):
                    print("CST returned as dictionary, creating deep copy...")
                    # We have a dictionary, make a deep copy using our safer function
                    # that ensures all values are pure Python types
                    cst_dict = cst_node_to_dict(cst_root)
                    print(f"Dictionary CST processed successfully with {len(cst_dict.get('children', []))} top-level children")
                else:
                    print("CST returned as object, converting to dictionary...")
                    # Create a safe dictionary copy of the CST tree using the legacy approach
                    cst_dict = cleanup_cst_tree(cst_root)
                    print("Object-based CST converted to dictionary successfully")
                
                # Explicitly dereference the CST root to help garbage collection
                cst_root = None
                
                # Force garbage collection to clean up CST objects early
                print("Running garbage collection...")
                gc.collect()
                print("Garbage collection complete.")
                
                # Verify we have a valid dictionary with required fields
                if not isinstance(cst_dict, dict):
                    print(f"Warning: cst_dict is not a dictionary but {type(cst_dict)}")
                    cst_dict = {
                        "type": "ERROR", 
                        "content": "", 
                        "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
                        "children": []
                    }
                    
                # Ensure all required fields exist
                for required_field in ["type", "content", "range", "children"]:
                    if required_field not in cst_dict:
                        print(f"Warning: Missing required field '{required_field}' in CST")
                        if required_field == "range":
                            cst_dict[required_field] = {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}}
                        elif required_field == "children":
                            cst_dict[required_field] = []
                        else:
                            cst_dict[required_field] = ""
            except Exception as e:
                print(f"Error processing CST: {e}")
                # Create a minimal valid structure
                cst_dict = {
                    "type": "ERROR", 
                    "content": f"Exception: {str(e)}", 
                    "range": {"start": {"line": 0, "column": 0}, "end": {"line": 0, "column": 0}},
                    "children": []
                }
        
        # Extract main node from AST if available for docstring processing
        main_node = None
        if ast_dict is not None:
            # Find the main function in the ast_dict
            for node in ast_dict.get("children", []):
                if node.get("name") == "main":
                    main_node = node
                    break
                    
        # Add docstring info if available
        if main_node and main_node.get("docstring"):
            doc_start = main_node["range"]["start"]["line"] - 4
            doc_end = main_node["range"]["start"]["line"] - 1
            cst_dict.get("children", []).append({
                "type": "comment",
                "content": main_node["docstring"],
                "range": {
                    "start": {"line": doc_start, "column": 0},
                    "end": {"line": doc_end, "column": 3}
                },
                "children": []
            })

        # Run a final garbage collection before returning
        gc.collect()
        
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
    import gc  # Import garbage collector for explicit cleanup
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
        # Ensure we're saving in the examples directory, not the tests directory
        dir_path = os.path.dirname(file_path)
        
        # Check if we're already in examples directory
        if "/examples/" in dir_path or dir_path.endswith("/examples"):
            # Already in examples directory, keep as is
            output_dir = dir_path
        # Handle files in core/tests/c that should go to core/tests/examples/c
        elif "/core/tests/c/" in dir_path or dir_path.endswith("/core/tests/c"):
            # Convert path from core/tests/c to core/tests/examples/c
            output_dir = dir_path.replace("/core/tests/c", "/core/tests/examples/c")
            os.makedirs(output_dir, exist_ok=True)
        else:
            # Default case - use current directory
            output_dir = dir_path
            
        output_path = os.path.join(output_dir, output_file_name)

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
        gc.collect()  # Force garbage collection even when failing
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

    # Clean up by explicitly removing references before writing
    result_dict = None
    new_json_obj = json.loads(new_json)
    new_json_obj = json.dumps(new_json_obj, indent=2, sort_keys=True)
    
    # Force an aggressive garbage collection before file operations
    gc.collect()
    
    # Write the output file
    if not dry_run:
        try:
            with open(output_path, "w") as f:
                f.write(new_json)
            if verbose:
                print(f"Wrote {output_path}")
            # Final cleanup before returning
            ast_dict = None
            cst_dict = None
            new_json = None
            gc.collect()
            return True
        except Exception as e:
            print(f"Error writing {output_path}: {e}")
            gc.collect()
            return False
    else:
        if verbose:
            print(f"Would write {output_path} (dry run)")
        # Final cleanup before returning
        ast_dict = None
        cst_dict = None
        new_json = None
        gc.collect()
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


def cleanup_resources():
    """Perform explicit cleanup of resources before exit to avoid segfaults."""
    global _processed_nodes
    
    print("Performing final cleanup before exit...")
    # Clear any global collections that might reference CST nodes
    _processed_nodes.clear()
    
    # Force garbage collection multiple times
    print("Running deep garbage collection...")
    for i in range(3):
        gc.collect()
    print("Deep garbage collection complete")


if __name__ == "__main__":
    try:
        main()
        print("Main execution completed successfully")
    except Exception as e:
        print(f"Error in main execution: {e}")
    finally:
        # Always run cleanup regardless of success or failure
        cleanup_resources()
        print("Program exiting normally")
