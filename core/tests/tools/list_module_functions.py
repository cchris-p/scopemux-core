#!/usr/bin/env python3
"""
List all functions available in the scopemux_core module
"""

import sys
import os
import inspect

# Set up Python path
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
sys.path.insert(0, project_root)

try:
    print("Importing scopemux_core module...")
    import scopemux_core
    print("Successfully imported scopemux_core module")
    
    print("\nAvailable functions and attributes in scopemux_core:")
    for name in dir(scopemux_core):
        if not name.startswith('__'):
            attr = getattr(scopemux_core, name)
            if inspect.isfunction(attr) or inspect.isbuiltin(attr):
                print(f"  {name} (function)")
            else:
                print(f"  {name} ({type(attr).__name__})")
    
except Exception as e:
    print(f"Error: {e}")
