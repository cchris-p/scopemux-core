# ScopeMux Example Suite Documentation

This document outlines the structure and purpose of the example suite used to test the ScopeMux parser across **C**, **C++**, and **Python**.
Each source file is paired with an `*.expected.json` file that describes the **expected parser output**, including AST structure, symbol extraction, comments, and more.

---

## 📁 Directory Structure

```
/home/matrillo/apps/scopemux/bindings/examples/
├── c/
│   ├── basic_syntax/
│   ├── complex_structures/
│   ├── function_definitions/
│   ├── struct_union_enum/
│   ├── comments/
│   ├── preprocessor_directives/
│   ├── memory_management/
│   └── file_io/
├── cpp/
│   ├── basic_syntax/
│   ├── complex_structures/
│   ├── function_definitions/
│   ├── class_definitions/
│   ├── comments_docstrings/
│   ├── templates/
│   ├── namespaces/
│   ├── operator_overloading/
│   ├── stl_usage/
│   └── error_handling/
└── python/
    ├── basic_syntax/
    ├── complex_structures/
    ├── function_definitions/
    ├── class_definitions/
    ├── comments_docstrings/
    ├── decorators/
    ├── comprehensions_generators/
    ├── modules_imports/
    ├── error_handling/
    └── advanced_features/
```

---

## 🔍 Example Categories by Language

### ✅ C Examples (`/c/`)

| Category                  | Example Files                                            | Focus                                     |
| ------------------------- | -------------------------------------------------------- | ----------------------------------------- |
| `basic_syntax`            | `hello_world.c`, `variables_loops_conditions.c`          | Includes, `main()`, control structures    |
| `complex_structures`      | `nested_control_flow_goto.c`, `arrays_pointers.c`        | Deep nesting, pointers, arrays            |
| `function_definitions`    | `multiple_functions_prototypes.c`, `function_pointers.c` | Signatures, prototypes, function pointers |
| `struct_union_enum`       | `struct_usage.c`, `union_usage.c`, `enum_usage.c`        | Structs, unions, enums                    |
| `comments`                | `various_comments.c`                                     | Inline, block comments                    |
| `preprocessor_directives` | `macros_includes.c`, `conditional_compilation.c`         | `#define`, `#if`, `#include`, etc.        |
| `memory_management`       | `malloc_free.c`                                          | `malloc`, `calloc`, `free`                |
| `file_io`                 | `basic_file_operations.c`                                | `fopen`, `fread`, `fprintf`, etc.         |

---

### ✅ C++ Examples (`/cpp/`)

| Category               | Example Files                                                                          | Focus                                   |
| ---------------------- | -------------------------------------------------------------------------------------- | --------------------------------------- |
| `basic_syntax`         | `hello_world.cpp`, `variables_loops_conditions.cpp`                                    | `iostream`, `std::cout`, modern loops   |
| `complex_structures`   | `nested_loops_ifs_switch.cpp`, `complex_data_structures.cpp`                           | Nesting, STL containers                 |
| `function_definitions` | `function_overloading.cpp`, `advanced_signatures.cpp`                                  | Overloading, `const`, `static`          |
| `class_definitions`    | `simple_class_members.cpp`, `inheritance_polymorphism.cpp`, `multiple_inheritance.cpp` | Classes, inheritance, virtual functions |
| `comments_docstrings`  | `various_comments_doxygen.cpp`                                                         | Doxygen-style documentation             |
| `templates`            | `function_template.cpp`, `class_template.cpp`, `template_specialization.cpp`           | Template syntax and specialization      |
| `namespaces`           | `basic_namespace.cpp`, `nested_anonymous_namespaces.cpp`                               | Namespaces and scoping                  |
| `operator_overloading` | `arithmetic_operators.cpp`, `stream_operators.cpp`                                     | Custom operator handling                |
| `stl_usage`            | `common_stl_containers_algorithms.cpp`                                                 | Lambdas, STL algorithms                 |
| `error_handling`       | `try_catch_throw.cpp`                                                                  | `try/catch`, exceptions                 |

---

### ✅ Python Examples (`/python/`)

| Category                    | Example Files                                                             | Focus                                    |
| --------------------------- | ------------------------------------------------------------------------- | ---------------------------------------- |
| `basic_syntax`              | `hello_world.py`, `variables_loops_conditions.py`                         | Print, control flow                      |
| `complex_structures`        | `nested_control_flow.py`, `complex_data_structures.py`                    | Loops, collections                       |
| `function_definitions`      | `various_signatures.py`, `lambda_functions.py`                            | `*args`, `**kwargs`, type hints, lambdas |
| `class_definitions`         | `simple_class_methods.py`, `inheritance_super_mro.py`, `magic_methods.py` | Classes, methods, MRO, dunder methods    |
| `comments_docstrings`       | `various_comments_docstrings_pep257.py`                                   | Comments, PEP 257-style docstrings       |
| `decorators`                | `simple_decorator.py`, `decorator_with_args.py`                           | Decorators and decorator factories       |
| `comprehensions_generators` | `list_dict_set_comprehensions.py`, `generator_functions_expressions.py`   | List/set/dict comprehensions, `yield`    |
| `modules_imports`           | `basic_imports.py`, `relative_imports/`                                   | Import statements and relative imports   |
| `error_handling`            | `try_except_finally_raise.py`                                             | Try/except, raising exceptions           |
| `advanced_features`         | `context_managers.py`, `async_await.py`                                   | Context managers, async/await            |

---

## 📦 Integration Strategy

Each example source file (e.g., `example.c`) is accompanied by an expected output file:

```text
example.c.expected.json
```

This file defines the **target AST structure**, functions, classes, comments, etc., expected from the parser.

Your test harness will:

1. Load and parse the source file.
2. Generate the parser output.
3. Compare it against the `.expected.json` content.
