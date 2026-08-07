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
    
    # Check if memory debugging is available
    if hasattr(scopemux_core, 'memory_debug_configure'):
        logging.info("Configuring memory debugging...")
        scopemux_core.memory_debug_configure(True, True, True)
        scopemux_core.memory_debug_init()
        logging.info("Memory debugging initialized")
    
    # Initialize parser context
    logging.info("Creating parser context...")
    ctx = scopemux_core.parser_context_new()
    logging.info(f"Parser context created: {ctx}")
    
    # Step-by-step debugging
    logging.info("Step 1: Initializing parser...")
    result = scopemux_core.parser_init(ctx)
    logging.info(f"Parser initialization result: {result}")
    
    # Simple C code to parse
    c_code = """
    #include <stdio.h>
    
    int main() {
        printf("Hello, World!\\n");
        return 0;
    }
    """
    
    logging.info("Step 2: Setting language to C...")
    scopemux_core.parser_set_language(ctx, 1)  # 1 = C
    
    logging.info("Step 3: Parsing string...")
    try:
        # This is where the segfault might occur
        logging.info("About to call parser_parse_string...")
        result = scopemux_core.parser_parse_string(ctx, c_code)
        logging.info(f"Parse result: {result}")
        
        logging.info("Step 4: Getting AST...")
        ast = scopemux_core.parser_get_ast(ctx)
        logging.info(f"AST result: {ast}")
        
    except Exception as e:
        logging.error(f"Exception during parsing: {e}")
        traceback.print_exc()
    
    # Clean up
    logging.info("Cleaning up parser context...")
    scopemux_core.parser_context_free(ctx)
    logging.info("Parser context freed")
    
except Exception as e:
    logging.error(f"Exception: {e}")
    traceback.print_exc()

logging.info("Script completed")
