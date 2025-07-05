#!/usr/bin/env python3
"""
Debug script for testing parser initialization and AST/CST conversion.
"""

import os
import sys
import gc
import signal
import traceback

# Set up Python path to find both the core module and the parser_testing module
project_root = os.path.abspath(os.path.dirname(__file__))
core_build_path = os.path.join(project_root, "build/lib.linux-x86_64-3.10")
core_direct_path = os.path.join(project_root)

# Add paths to sys.path if they're not already there
for path in [core_direct_path, core_build_path]:
    if path not in sys.path:
        sys.path.insert(0, path)

# Define a segfault handler
def debug_segfault_handler(signum, frame):
    print("\n*** SEGMENTATION FAULT DETECTED ***")
    print("Stack trace at point of failure:")
    traceback.print_stack(frame)
    sys.exit(1)

# Register the handler
signal.signal(signal.SIGSEGV, debug_segfault_handler)

def main():
    try:
        # Import the core module
        print("Attempting to import scopemux_core...")
        import scopemux_core
        print("Successfully imported scopemux_core")
        
        # Print available classes and functions for debugging
        print("\nAvailable items in scopemux_core:")
        items = [attr for attr in dir(scopemux_core) if not attr.startswith('_')]
        for item in items:
            print(f"  - {item}")
        
        # Test file path
        test_file = "core/tests/examples/c/basic_syntax/hello_world.c"
        print(f"\nTesting parser with file: {test_file}")
        
        # Read the file content
        with open(test_file, 'r') as f:
            content = f.read()
        
        print("Step 1: Creating a ParserContext")
        parser_context = scopemux_core.ParserContext()
        print(f"Parser context created: {parser_context}")
        
        print("Step 2: Detecting language from file extension")
        language_id = scopemux_core.detect_language(test_file)
        print(f"Detected language ID: {language_id}")
        
        print("Step 3: Parsing source string")
        try:
            # Convert language_id to string representation
            language_str = "C" if language_id == 1 else \
                          "C++" if language_id == 2 else \
                          "Python" if language_id == 3 else \
                          "JavaScript" if language_id == 4 else \
                          "TypeScript" if language_id == 5 else "Unknown"
            
            print(f"Using language string: {language_str}")
            result = parser_context.parse_string(content, language_str)
            print(f"Parse result: {result}")
        except Exception as e:
            print(f"Error parsing source string: {e}")
            traceback.print_exc()
        
        print("Step 4: Getting AST root")
        try:
            ast_root = parser_context.get_ast_root()
            print(f"AST root retrieved: {ast_root}")
        except Exception as e:
            print(f"Error getting AST root: {e}")
            traceback.print_exc()
        
        print("Step 5: Getting CST root")
        try:
            cst_root = parser_context.get_cst_root()
            print(f"CST root retrieved: {cst_root}")
        except Exception as e:
            print(f"Error getting CST root: {e}")
            traceback.print_exc()
        
        print("Step 6: Getting AST as dictionary")
        try:
            if ast_root:
                # Convert the AST node to a dictionary
                ast_dict = {}
                
                # Check if the node has methods to access its properties
                if hasattr(ast_root, 'get_type'):
                    ast_dict['type'] = ast_root.get_type()
                
                if hasattr(ast_root, 'get_children'):
                    children = ast_root.get_children()
                    ast_dict['children'] = [f"Child {i}" for i in range(len(children))]
                    print(f"AST has {len(children)} children")
                
                print(f"AST dictionary created with keys: {list(ast_dict.keys())}")
            else:
                print("No AST root available")
        except Exception as e:
            print(f"Error converting AST to dictionary: {e}")
            traceback.print_exc()
        
        print("Step 7: Getting CST as dictionary")
        try:
            if cst_root:
                # Convert the CST node to a dictionary
                cst_dict = {}
                
                # Check if the node has methods to access its properties
                if hasattr(cst_root, 'get_type'):
                    cst_dict['type'] = cst_root.get_type()
                
                if hasattr(cst_root, 'get_children'):
                    children = cst_root.get_children()
                    cst_dict['children'] = [f"Child {i}" for i in range(len(children))]
                    print(f"CST has {len(children)} children")
                
                print(f"CST dictionary created with keys: {list(cst_dict.keys())}")
            else:
                print("No CST root available")
        except Exception as e:
            print(f"Error converting CST to dictionary: {e}")
            traceback.print_exc()
        
        print("Step 8: Cleanup")
        # ParserContext should be automatically cleaned up by Python's garbage collection
        parser_context = None
        print("Set parser_context to None for cleanup")
        
    except ImportError as e:
        print(f"Error importing scopemux_core: {e}")
        print("\nTroubleshooting steps:")
        print("1. Make sure the scopemux_core module is built and in the Python path")
        print("2. Run 'build_all_and_pybind.sh' from the project root directory")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    try:
        main()
    finally:
        # Always run cleanup
        print("\nPerforming final cleanup before exit...")
        gc.collect()
        print("Program exiting")
