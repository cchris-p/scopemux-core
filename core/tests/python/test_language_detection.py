#!/usr/bin/env python3
"""
Test script for language detection in ScopeMux
This script demonstrates the language detection functionality in ScopeMux
"""

import os
import sys
import tempfile
from pathlib import Path

# Add the project root directory to sys.path to import scopemux_core
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
core_build_path = os.path.join(project_root, "build/core")
if core_build_path not in sys.path:
    sys.path.insert(0, core_build_path)

try:
    import scopemux_core as sm

    print(f"Successfully imported scopemux_core from {sm.__file__}")
except ImportError as e:
    print(f"Error: Failed to import scopemux_core. {e}")
    print(f"Current sys.path: {sys.path}")
    print(f"Looking for .so files in project root:")
    for file in os.listdir(project_root):
        if file.endswith(".so"):
            print(f"  - {file}")
    sys.exit(1)


def test_language_detection():
    """Test language detection functionality"""
    print("Testing language detection functionality...")

    # Create temporary files for testing
    with tempfile.TemporaryDirectory() as temp_dir:
        # C file
        c_file_path = os.path.join(temp_dir, "test.c")
        c_code = """
        #include <stdio.h>
        
        int main() {
            printf("Hello, world!\\n");
            return 0;
        }
        """
        with open(c_file_path, "w") as f:
            f.write(c_code)

        # C++ file
        cpp_file_path = os.path.join(temp_dir, "test.cpp")
        cpp_code = """
        #include <iostream>
        
        int main() {
            std::cout << "Hello, world!" << std::endl;
            return 0;
        }
        """
        with open(cpp_file_path, "w") as f:
            f.write(cpp_code)

        # Python file
        py_file_path = os.path.join(temp_dir, "test.py")
        py_code = """
        def main():
            print("Hello, world!")
            
        if __name__ == "__main__":
            main()
        """
        with open(py_file_path, "w") as f:
            f.write(py_code)

        # Test language detection by file extension
        print(f"\nTesting language detection by file extension:")
        print(f"C file: {c_file_path}")
        c_lang = sm.detect_language(c_file_path)
        print(f"Detected language: {language_to_string(c_lang)}")

        print(f"\nC++ file: {cpp_file_path}")
        cpp_lang = sm.detect_language(cpp_file_path)
        print(f"Detected language: {language_to_string(cpp_lang)}")

        print(f"\nPython file: {py_file_path}")
        py_lang = sm.detect_language(py_file_path)
        print(f"Detected language: {language_to_string(py_lang)}")

        # Test language detection with content
        print(f"\nTesting language detection with content:")

        # File with .txt extension but C content
        txt_c_file_path = os.path.join(temp_dir, "c_code.txt")
        with open(txt_c_file_path, "w") as f:
            f.write(c_code)

        print(f"Text file with C content: {txt_c_file_path}")
        with open(txt_c_file_path, "r") as f:
            content = f.read()
        txt_c_lang = sm.detect_language(txt_c_file_path, content)
        print(f"Detected language: {language_to_string(txt_c_lang)}")

        # File with .txt extension but Python content
        txt_py_file_path = os.path.join(temp_dir, "py_code.txt")
        with open(txt_py_file_path, "w") as f:
            f.write(py_code)

        print(f"Text file with Python content: {txt_py_file_path}")
        with open(txt_py_file_path, "r") as f:
            content = f.read()
        txt_py_lang = sm.detect_language(txt_py_file_path, content)
        print(f"Detected language: {language_to_string(txt_py_lang)}")


def language_to_string(lang_type):
    """Convert language type enum to string"""
    lang_map = {
        sm.LANG_UNKNOWN: "Unknown",
        sm.LANG_C: "C",
        sm.LANG_CPP: "C++",
        sm.LANG_PYTHON: "Python",
        sm.LANG_JAVASCRIPT: "JavaScript",
        sm.LANG_TYPESCRIPT: "TypeScript",
    }
    return lang_map.get(lang_type, f"Unknown ({lang_type})")


def test_parsing_with_language_detection():
    """Test parsing with language detection"""
    print("\nTesting parsing with language detection...")

    # Create a parser context
    ctx = sm.ParserContext()

    # Create temporary files for testing
    with tempfile.TemporaryDirectory() as temp_dir:
        # C file
        c_file_path = os.path.join(temp_dir, "test.c")
        c_code = """
        #include <stdio.h>
        
        /**
         * Add two integers
         */
        int add(int a, int b) {
            return a + b;
        }
        
        int main() {
            int result = add(5, 3);
            printf("Result: %d\\n", result);
            return 0;
        }
        """
        with open(c_file_path, "w") as f:
            f.write(c_code)

        # Python file
        py_file_path = os.path.join(temp_dir, "test.py")
        py_code = """
        '''
        A simple Python module
        '''
        
        def add(a, b):
            '''Add two numbers'''
            return a + b
            
        class Calculator:
            '''A simple calculator class'''
            
            def __init__(self):
                self.result = 0
                
            def add(self, a, b):
                '''Add two numbers and store the result'''
                self.result = a + b
                return self.result
        
        if __name__ == "__main__":
            result = add(5, 3)
            print(f"Result: {result}")
            
            calc = Calculator()
            calc_result = calc.add(10, 20)
            print(f"Calculator result: {calc_result}")
        """
        with open(py_file_path, "w") as f:
            f.write(py_code)

        # Parse C file with automatic language detection
        print(f"\nParsing C file: {c_file_path}")
        ctx.parse_file(c_file_path)

        # Get function nodes
        c_functions = ctx.get_nodes_by_type(sm.NODE_FUNCTION)
        print(f"Found {len(c_functions)} functions in C file:")
        for func in c_functions:
            print(f"  - {func.name}: {func.signature}")
            if func.docstring:
                print(f"    Docstring: {func.docstring}")

        # Parse Python file with automatic language detection
        print(f"\nParsing Python file: {py_file_path}")
        ctx.parse_file(py_file_path)

        # Get function and class nodes
        py_functions = ctx.get_nodes_by_type(sm.NODE_FUNCTION)
        py_classes = ctx.get_nodes_by_type(sm.NODE_CLASS)

        print(f"Found {len(py_functions)} functions in Python file:")
        for func in py_functions:
            print(f"  - {func.name}: {func.signature}")
            if func.docstring:
                print(f"    Docstring: {func.docstring}")

        print(f"Found {len(py_classes)} classes in Python file:")
        for cls in py_classes:
            print(f"  - {cls.name}")
            if cls.docstring:
                print(f"    Docstring: {cls.docstring}")


if __name__ == "__main__":
    print("ScopeMux Language Detection Test")
    print("================================\n")

    test_language_detection()
    test_parsing_with_language_detection()

    print("\nAll tests completed successfully!")
