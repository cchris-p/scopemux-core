#!/usr/bin/env python3
"""
Debug script for ScopeMux parser segmentation fault
"""

import sys
import os
import gc
import signal
import traceback
import logging
import ctypes

# Configure logging
logging.basicConfig(level=logging.DEBUG, 
                   format='[%(asctime)s] [%(levelname)s] %(message)s',
                   datefmt='%Y-%m-%d %H:%M:%S')

# Set up Python path
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
sys.path.insert(0, project_root)

# Define segfault handler
def segfault_handler(signum, frame):
    logging.error("SEGMENTATION FAULT DETECTED")
    logging.error("Current stack trace:")
    traceback.print_stack(frame)
    sys.exit(1)

# Register signal handler for SIGSEGV
signal.signal(signal.SIGSEGV, segfault_handler)

try:
    logging.info("Attempting to import scopemux_core module...")
    import scopemux_core
    logging.info("Successfully imported scopemux_core module")
    
    # Try to get the segfault handler from the module
    if hasattr(scopemux_core, '_segfault_handler'):
        logging.info("Found segfault_handler in scopemux_core module")
    
    # Initialize parser context
    logging.info("Creating parser context...")
    ctx = scopemux_core.ParserContext()
    logging.info(f"Parser context created: {ctx}")
    
    # Simple C code to parse - very minimal to reduce complexity
    c_code = "int main() { return 0; }"
    
    # Inspect the ParserContext object
    logging.info(f"ParserContext methods: {dir(ctx)}")
    
    # Try to parse the C code step by step
    logging.info("Attempting to parse C code...")
    try:
        # Force garbage collection before parsing
        gc.collect()
        
        # Use C language directly instead of detection
        lang_str = "c"
        logging.info(f"Using language string: {lang_str}")
        
        # Try to parse using the C language
        logging.info("Parsing code with language c...")
        
        # Check if parse_string method exists
        if hasattr(ctx, 'parse_string'):
            # Try to parse with minimal code
            logging.info("About to call parse_string...")
            result = ctx.parse_string(c_code, lang_str)
            logging.info(f"Parse result: {result}")
            
            # If we get here, parsing succeeded
            logging.info("Parsing succeeded, getting CST root...")
            if hasattr(ctx, 'get_cst_root'):
                cst = ctx.get_cst_root()
                logging.info(f"CST root: {cst}")
            
            # Try to get the AST root
            logging.info("Getting AST root...")
            if hasattr(ctx, 'get_ast_root'):
                ast = ctx.get_ast_root()
                logging.info(f"AST root: {ast}")
        else:
            logging.error("parse_string method not found in ParserContext")
            logging.info("Available methods: " + ", ".join(dir(ctx)))
        
    except Exception as e:
        logging.error(f"Exception during parsing: {e}")
        traceback.print_exc()
    
    # Clean up
    logging.info("Cleaning up parser context...")
    del ctx
    gc.collect()
    logging.info("Parser context cleaned up")
    
except Exception as e:
    logging.error(f"Exception: {e}")
    traceback.print_exc()

logging.info("Script completed")
