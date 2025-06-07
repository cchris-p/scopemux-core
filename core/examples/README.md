# ScopeMux Code Examples

This directory contains code examples in multiple languages for testing and demonstrating the ScopeMux parsing capabilities. These examples are designed to cover a wide range of language features and are paired with expected output files that describe the expected AST structure.

## Directory Structure

The examples are organized by language and feature:

```
examples/
├── c/
│   ├── basic_syntax/
│   │   ├── hello_world.c
│   │   ├── hello_world.c.expected.json
│   │   ├── variables_loops_conditions.c
│   │   └── variables_loops_conditions.c.expected.json
│   ├── complex_structures/
│   │   ├── arrays_pointers.c
│   │   ├── arrays_pointers.c.expected.json
│   │   ├── nested_control_flow_goto.c
│   │   └── nested_control_flow_goto.c.expected.json
│   ├── file_io/
│   │   ├── file_operations.c
│   │   └── file_operations.c.expected.json
│   ├── memory_management/
│   │   ├── dynamic_memory.c
│   │   └── dynamic_memory.c.expected.json
│   └── struct_union_enum/
│       ├── complex_data_types.c
│       └── complex_data_types.c.expected.json
├── cpp/
│   ├── basic_syntax/
│   │   ├── hello_world.cpp
│   │   ├── hello_world.cpp.expected.json
│   │   ├── variables_loops_conditions.cpp
│   │   └── variables_loops_conditions.cpp.expected.json
│   ├── complex_structures/
│   │   ├── complex_data_structures.cpp
│   │   ├── complex_data_structures.cpp.expected.json
│   │   ├── nested_loops_ifs_switch.cpp
│   │   └── nested_loops_ifs_switch.cpp.expected.json
│   ├── modern_cpp/
│   │   ├── modern_cpp_features.cpp
│   │   └── modern_cpp_features.cpp.expected.json
│   └── templates/
│       ├── advanced_templates.cpp
│       ├── advanced_templates.cpp.expected.json
│       ├── templates_basics.cpp
│       └── templates_basics.cpp.expected.json
└── python/
    ├── basic_syntax/
    │   ├── hello_world.py
    │   ├── hello_world.py.expected.json
    │   ├── variables_loops_conditions.py
    │   └── variables_loops_conditions.py.expected.json
    ├── complex_structures/
    │   ├── complex_data_structures.py
    │   └── complex_data_structures.py.expected.json
    ├── comprehensions_generators/
    │   ├── comprehensions.py
    │   ├── comprehensions.py.expected.json
    │   ├── generators.py
    │   └── generators.py.expected.json
    ├── decorators/
    │   ├── decorator_examples.py
    │   └── decorator_examples.py.expected.json
    └── metaprogramming/
        ├── metaclasses.py
        └── metaclasses.py.expected.json
```

## Example Types

### C Language Examples

- **Basic Syntax**: Simple examples demonstrating core C syntax elements.
- **Complex Structures**: Examples showing nested control flow, goto statements, arrays, and pointers.
- **File I/O**: Examples of file operations like reading, writing, and seeking.
- **Memory Management**: Examples of dynamic memory allocation, manipulation, and management.
- **Struct, Union, Enum**: Examples of complex data types and their usage.

### C++ Language Examples

- **Basic Syntax**: Simple examples demonstrating core C++ syntax elements.
- **Complex Structures**: Examples showing nested control structures and complex data structures.
- **Modern C++**: Examples of C++11/14/17 features like auto, lambdas, smart pointers, etc.
- **Templates**: Examples of function templates, class templates, and template metaprogramming.

### Python Language Examples

- **Basic Syntax**: Simple examples demonstrating core Python syntax elements.
- **Complex Structures**: Examples showing nested data structures and object-oriented patterns.
- **Comprehensions & Generators**: Examples of list/set/dict comprehensions and generator functions/expressions.
- **Decorators**: Examples of function and class decorators.
- **Metaprogramming**: Examples of metaclasses, descriptors, and dynamic class creation.

## Expected Output Files

Each example code file is paired with an `.expected.json` file that contains the expected Abstract Syntax Tree (AST) structure that should be produced when parsing the example. These files serve as a reference for validating the parser's output.

The expected output files follow this general structure:

```json
{
  "language": "Language name",
  "ast": {
    "type": "translation_unit|module",
    "functions": [...],
    "classes": [...],
    "includes|imports": [...],
    "docstrings": [...],
    ...
  }
}
```

## Usage

These examples can be used for:

1. **Testing the Parser**: Verifying that the ScopeMux parser correctly handles various language features.
2. **Development Reference**: Understanding how different language constructs should be represented in the AST.
3. **Regression Testing**: Ensuring that changes to the parser don't break existing functionality.

To test the parser against these examples, parse each example file and compare the resulting AST against the expected output in the corresponding `.expected.json` file.

## Contributing New Examples

When adding new examples:

1. Create the example code file in the appropriate language/feature directory.
2. Create a corresponding `.expected.json` file with the expected AST structure.
3. Ensure the example demonstrates a specific language feature or edge case.
4. Update this README if you add a new feature category.
