#!/usr/bin/env python3
"""
Enhanced debug script with memory debugging tools for testing parser initialization and AST/CST conversion.
"""

import os
import sys
import gc
import signal
import ctypes
import traceback

# Set up Python path to find both the core module and the parser_testing module
project_root = os.path.abspath(os.path.dirname(__file__))
core_build_path = os.path.join(project_root, "build/lib.linux-x86_64-3.10")
core_direct_path = os.path.join(project_root)

# Add paths to sys.path if they're not already there
for path in [core_direct_path, core_build_path]:
    if path not in sys.path:
        sys.path.insert(0, path)

# Define an enhanced segfault handler with more diagnostics
def debug_segfault_handler(signum, frame):
    print("\n*** SEGMENTATION FAULT DETECTED ***")
    print("Stack trace at point of failure:")
    traceback.print_stack(frame)
    
    print("\nDetailed traceback:")
    traceback.print_exc()
    
    print("\nProcess information:")
    import psutil
    process = psutil.Process(os.getpid())
    print(f"Memory usage: {process.memory_info().rss / 1024 / 1024:.2f} MB")
    
    sys.exit(1)

# Register the handler
signal.signal(signal.SIGSEGV, debug_segfault_handler)

def setup_crash_handler(scopemux_core):
    """Try to setup the C crash handler if available"""
    try:
        if hasattr(scopemux_core, 'setup_crash_handler'):
            print("Setting up ScopeMux crash handler...")
            scopemux_core.setup_crash_handler()
            return True
        return False
    except Exception as e:
        print(f"Failed to setup crash handler: {e}")
        return False

def main():
    try:
        # Import the core module
        print("Attempting to import scopemux_core...")
        import scopemux_core
        print("Successfully imported scopemux_core")
        
        # Setup crash handler if available
        crash_handler_enabled = setup_crash_handler(scopemux_core)
        if not crash_handler_enabled:
            print("ScopeMux crash handler not available, using Python handler")
            
        # Try to locate the segfault_handler symbol if it exists
        try:
            # Find the library path
            library_path = None
            for module in sys.modules.values():
                if hasattr(module, '__file__') and module.__file__ and 'scopemux_core' in module.__file__:
                    library_path = module.__file__
                    break
            
            if library_path:
                print(f"Found scopemux_core library at: {library_path}")
                if os.path.exists(library_path):
                    # Try to load the segfault_handler symbol
                    try:
                        lib = ctypes.CDLL(library_path)
                        if hasattr(lib, 'segfault_handler'):
                            print("Found segfault_handler in library")
                    except Exception as e:
                        print(f"Error loading library: {e}")
        except Exception as e:
            print(f"Error searching for segfault_handler: {e}")
        
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
            print(f"Loaded source file: {len(content)} bytes")
        
        print("\n======== INCREMENTAL PARSING APPROACH ========")
        try:
            print("Step 1: Creating a ParserContext")
            parser_context = scopemux_core.ParserContext()
            print(f"Parser context created: {parser_context}")
            
            print("Step 2: Detecting language from file extension")
            language_id = scopemux_core.detect_language(test_file)
            print(f"Detected language ID: {language_id}")
            
            # Map language ID to string (use lowercase as expected by parser bindings)
            language_str = "c" if language_id == 1 else \
                          "cpp" if language_id == 2 else \
                          "python" if language_id == 3 else \
                          "javascript" if language_id == 4 else \
                          "typescript" if language_id == 5 else "unknown"
            print(f"Using language string: {language_str}")
            
            # Force garbage collection before parsing
            print("Running garbage collection before parsing...")
            gc.collect()
            
            print("\nStep 3: Parsing source string")
            print("Setting up memory debugger...")
            try:
                # Try setting up memory debugging if available
                if hasattr(scopemux_core, 'enable_memory_debugging'):
                    scopemux_core.enable_memory_debugging(True)
                    print("Memory debugging enabled")
            except Exception as e:
                print(f"Warning: Failed to enable memory debugging: {e}")
                
            print("About to call parse_string...")
            result = parser_context.parse_string(content, language_str)
            print(f"Parse result: {result}")
            
            print("\nStep 4: Getting AST root")
            ast_root = parser_context.get_ast_root()
            print(f"AST root retrieved: {ast_root}")
            
            print("\nStep 5: Getting CST root")
            cst_root = parser_context.get_cst_root() 
            print(f"CST root retrieved: {cst_root}")
            
            print("\nSuccessfully completed all parsing steps")
        except Exception as e:
            print(f"\nError during incremental parsing: {e}")
            traceback.print_exc()
        
        print("\n======== CLEANUP ========")
        # Force cleanup
        print("Cleaning up parser context...")
        if 'parser_context' in locals():
            parser_context = None
        
        # Force garbage collection
        print("Running final garbage collection...")
        gc.collect()
        print("Cleanup complete")
        
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
