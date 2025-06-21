#!/usr/bin/env python3
"""
Simple test script to verify the scopemux Python bindings are working
"""

try:
    import scopemux_core
    print("Successfully imported scopemux_core module")
    
    # Try to use detect_language function
    lang = scopemux_core.detect_language("test.py")
    print(f"Detected language for 'test.py': {lang}")
    
    # Try to create a parser context
    parser_ctx = scopemux_core.ParserContext()
    print(f"Created ParserContext object: {parser_ctx}")
    
    print("All basic functionality tests passed!")
except Exception as e:
    print(f"Error testing scopemux_core: {e}")
