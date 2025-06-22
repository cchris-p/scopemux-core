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

# This is a bridge script that redirects to the new refactored module
# It maintains the same interface as the original script for backward compatibility

import sys
import os
import gc
import signal

# Set up Python path to find both the core module and the parser_testing module
project_root = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "../../"))
core_build_path = os.path.join(project_root, "build/lib.linux-x86_64-3.10")
core_direct_path = os.path.join(project_root)

# Add paths to sys.path if they're not already there
for path in [core_direct_path, core_build_path]:
    if path not in sys.path:
        sys.path.insert(0, path)

# Define a dummy segfault handler that can be used in case the real one isn't available


def dummy_segfault_handler(signum, frame):
    print("Warning: Using dummy segfault handler")
    raise RuntimeError("Segmentation fault detected (dummy handler)")


# Register the dummy handler
signal.signal(signal.SIGSEGV, dummy_segfault_handler)

try:
    # Try to import the core module to ensure it's available
    try:
        # First clear any previous import attempts
        if 'scopemux_core' in sys.modules:
            del sys.modules['scopemux_core']

        # Try to import the module
        import scopemux_core
        print("Successfully imported scopemux_core module")
    except ImportError as e:
        print(f"Warning: Failed to preload scopemux_core: {e}")
        print("Will try again through the parser_testing module")

    # Import the new CLI module
    from core.parser_testing.cli import main
except ImportError as e:
    print(f"Error importing modules: {e}")
    print("\nTroubleshooting steps:")
    print("1. Make sure the core.parser_testing module is installed or in the Python path")
    print("2. Run 'build_all_and_pybind.sh' from the project root directory")
    print("3. Ensure the scopemux_core Python binding is properly built and accessible")
    print("\nPython path:")
    for p in sys.path:
        print(f"  {p}")
    sys.exit(1)

if __name__ == "__main__":
    try:
        # Run the main function from the new CLI module
        exit_code = main()
        sys.exit(exit_code)
    except Exception as e:
        print(f"Error in main execution: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
    finally:
        # Always run cleanup
        print("Performing final cleanup before exit...")
        gc.collect()
        print("Program exiting")
