#!/usr/bin/env python3

import os
import sys
import scopemux_core

def test_language_detection():
    """Test the language detection functionality"""
    print("Testing language detection...")
    
    # Test C file detection
    c_file = "example.c"
    c_lang = scopemux_core.detect_language(c_file)
    print(f"Detected language for {c_file}: {get_language_name(c_lang)}")
    
    # Test C++ file detection
    cpp_file = "example.cpp"
    cpp_lang = scopemux_core.detect_language(cpp_file)
    print(f"Detected language for {cpp_file}: {get_language_name(cpp_lang)}")
    
    # Test Python file detection
    py_file = "example.py"
    py_lang = scopemux_core.detect_language(py_file)
    print(f"Detected language for {py_file}: {get_language_name(py_lang)}")
    
    # Test unknown file detection
    unknown_file = "example.xyz"
    unknown_lang = scopemux_core.detect_language(unknown_file)
    print(f"Detected language for {unknown_file}: {get_language_name(unknown_lang)}")

def get_language_name(lang_type):
    """Convert language type enum to string"""
    if lang_type == scopemux_core.LANG_C:
        return "C"
    elif lang_type == scopemux_core.LANG_CPP:
        return "C++"
    elif lang_type == scopemux_core.LANG_PYTHON:
        return "Python"
    else:
        return "Unknown"

def test_parse_c_file():
    """Test parsing a C file"""
    print("\nTesting C file parsing...")
    parser = scopemux_core.ParserContext()
    
    # Create a simple C file for testing
    c_code = """
    #include <stdio.h>
    
    /**
     * A simple function that adds two numbers
     */
    int add(int a, int b) {
        return a + b;
    }
    
    int main() {
        printf("Hello, world!\n");
        int result = add(5, 3);
        printf("Result: %d\n", result);
        return 0;
    }
    """
    
    # Write the code to a temporary file
    with open("temp_c_test.c", "w") as f:
        f.write(c_code)
    
    # Parse the file
    success = parser.parse_file("temp_c_test.c")
    print(f"Parsing C file: {'success' if success else 'failed'}")
    if not success:
        print(f"Error: {parser.get_last_error()}")
    
    # Clean up
    os.remove("temp_c_test.c")

def test_parse_python_file():
    """Test parsing a Python file"""
    print("\nTesting Python file parsing...")
    parser = scopemux_core.ParserContext()
    
    # Create a simple Python file for testing
    py_code = """
    #!/usr/bin/env python3
    
    def add(a, b):
        '''A simple function that adds two numbers'''
        return a + b
    
    class Calculator:
        '''A simple calculator class'''
        
        def __init__(self):
            self.result = 0
        
        def add(self, a, b):
            self.result = a + b
            return self.result
    
    if __name__ == "__main__":
        print("Hello, world!")
        result = add(5, 3)
        print(f"Result: {result}")
        
        calc = Calculator()
        calc_result = calc.add(10, 20)
        print(f"Calculator result: {calc_result}")
    """
    
    # Write the code to a temporary file
    with open("temp_python_test.py", "w") as f:
        f.write(py_code)
    
    # Parse the file
    success = parser.parse_file("temp_python_test.py")
    print(f"Parsing Python file: {'success' if success else 'failed'}")
    if not success:
        print(f"Error: {parser.get_last_error()}")
    
    # Clean up
    os.remove("temp_python_test.py")

def main():
    """Main function to run all tests"""
    print("ScopeMux Multi-Language Support Test")
    print("===================================\n")
    
    test_language_detection()
    test_parse_c_file()
    test_parse_python_file()

if __name__ == "__main__":
    main()
# ...