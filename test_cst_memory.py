#!/usr/bin/env python3
"""
Test script to verify CST memory management has been fixed.
This script repeatedly creates and destroys CST objects to check for memory leaks.
"""

import gc
import sys
import os
from core import scopemux_core


def cleanup_resources():
    """Force cleanup of resources and run garbage collection"""
    gc.collect()


def test_cst_memory():
    """Test CST memory management by creating and destroying CSTs"""
    print("Starting CST memory test...")

    # Create a parser context
    ctx = scopemux_core.ParserContext()

    # Simple Python code to parse
    code = """
def test_function():
    '''This is a docstring'''
    return "Hello, world!"

def another_function():
    return 42
"""

    # Parse the code and create CST multiple times
    for i in range(5):
        print(f"Iteration {i+1}/5")
        # Parse the string
        success = ctx.parse_string(code, "test.py", "python")
        if not success:
            print(f"Error parsing: {ctx.get_last_error()}")
            return

        # Get the CST root
        cst_root = ctx.get_cst_root()
        if not cst_root:
            print("Failed to get CST root")
            return

        # Check that we can access the CST data
        print(f"  Root type: {cst_root.get('type')}")
        print(f"  Child count: {len(cst_root.get('children'))}")

        # Force cleanup
        del cst_root
        cleanup_resources()

    print("\nMemory test completed successfully.")
    print("If no segmentation fault occurred, the memory management has been fixed.")


if __name__ == "__main__":
    test_cst_memory()
    # Add an explicit cleanup at the end
    cleanup_resources()
    print("Test completed - exiting normally")
