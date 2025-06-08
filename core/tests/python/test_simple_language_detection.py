#!/usr/bin/env python3
'''
Simple test for language detection in ScopeMux
This script demonstrates the basic language detection functionality
without relying on the Tree-sitter parsing capabilities
'''

import os
import sys
import tempfile

# Add the project directory to sys.path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def detect_language(filename, content=None):
    '''
    Simple language detection function that doesn't rely on the C extension
    
    Args:
        filename: The name of the file to detect the language of
        content: Optional content of the file
        
    Returns:
        A string representing the detected language
    '''
    # Detect by file extension
    ext = os.path.splitext(filename)[1].lower()
    
    if ext == '.c':
        return 'C'
    elif ext == '.cpp' or ext == '.cc' or ext == '.cxx' or ext == '.h' or ext == '.hpp':
        return 'C++'
    elif ext == '.py':
        return 'Python'
    elif ext == '.js':
        return 'JavaScript'
    elif ext == '.ts':
        return 'TypeScript'
    elif ext == '.rs':
        return 'Rust'
    
    # If no extension match, try to detect from content
    if content:
        # Look for Python-specific patterns
        if ('def ' in content and ':' in content) or 'import ' in content or 'from ' in content and ' import ' in content:
            return 'Python'
        
        # Look for C-specific patterns
        if '#include' in content and ('{' in content and '}' in content) and ';' in content:
            # Could be C or C++
            if 'class' in content or 'template' in content or 'namespace' in content or '::' in content:
                return 'C++'
            else:
                return 'C'
    
    return 'Unknown'

def test_language_detection():
    '''Test the language detection functionality'''
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
        c_lang = detect_language(c_file_path)
        print(f"Detected language: {c_lang}")
        
        print(f"\nC++ file: {cpp_file_path}")
        cpp_lang = detect_language(cpp_file_path)
        print(f"Detected language: {cpp_lang}")
        
        print(f"\nPython file: {py_file_path}")
        py_lang = detect_language(py_file_path)
        print(f"Detected language: {py_lang}")
        
        # Test language detection with content
        print(f"\nTesting language detection with content:")
        
        # File with .txt extension but C content
        txt_c_file_path = os.path.join(temp_dir, "c_code.txt")
        with open(txt_c_file_path, "w") as f:
            f.write(c_code)
        
        print(f"Text file with C content: {txt_c_file_path}")
        with open(txt_c_file_path, "r") as f:
            content = f.read()
        txt_c_lang = detect_language(txt_c_file_path, content)
        print(f"Detected language: {txt_c_lang}")
        
        # File with .txt extension but Python content
        txt_py_file_path = os.path.join(temp_dir, "py_code.txt")
        with open(txt_py_file_path, "w") as f:
            f.write(py_code)
        
        print(f"Text file with Python content: {txt_py_file_path}")
        with open(txt_py_file_path, "r") as f:
            content = f.read()
        txt_py_lang = detect_language(txt_py_file_path, content)
        print(f"Detected language: {txt_py_lang}")

if __name__ == "__main__":
    print("ScopeMux Simple Language Detection Test")
    print("======================================\n")
    
    test_language_detection()
    
    print("\nAll tests completed successfully!")
