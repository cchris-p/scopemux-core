;; C++ Macro Definitions
;; This file contains Tree-sitter queries for extracting C++ macro constructs.
;; Follows conventions used in C and Python .scm files.

;; Macro definition
(preproc_def
  name: (identifier) @name
  value: (_) @value?) @macro

;; Macro function-like definition
(preproc_function_def
  name: (identifier) @name
  parameters: (preproc_params) @params
  value: (_) @value?) @macro_function

;; TODO: Add support for macro conditionals and undef directives.
