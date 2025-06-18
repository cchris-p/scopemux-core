```
/home/matrillo/apps/scopemux/core/tests/
├── examples/                           # Test case examples (existing structure)
│   ├── c/...                           # C test examples 
│   ├── python/...                      # Python test examples
│   └── ...                             # Other languages
│
├── include/                            # Test header files
│   ├── test_helpers.h                  # Common test helper functions
│   ├── c_test_helpers.h                # C-specific test helpers
│   ├── python_test_helpers.h           # Python-specific test helpers
│   └── json_validation.h               # Helpers for JSON validation
│
├── src/
│   ├── common/                         # Common test utilities
│   │   ├── test_helpers.c              # Shared test helper implementations
│   │   └── json_validation.c           # JSON validation implementation
│   │
│   ├── c/                              # C language tests
│   │   ├── c_ast_tests.c               # C AST extraction tests
│   │   ├── c_cst_tests.c               # C CST extraction tests 
│   │   ├── c_preprocessor_tests.c      # C preprocessor directive tests
│   │   └── c_test_helpers.c            # C-specific helper implementations
│   │
│   ├── python/                         # Python language tests
│   │   ├── python_ast_tests.c          # Python AST extraction tests
│   │   ├── python_cst_tests.c          # Python CST extraction tests
│   │   └── python_test_helpers.c       # Python-specific helpers
│   │
│   ├── init_parser_tests.c             # Parser initialization tests (existing)
│   └── ast_extraction_tests.c          # (To be replaced by language-specific tests)
│
└── CMakeLists.txt                      # Test build configuration
```
