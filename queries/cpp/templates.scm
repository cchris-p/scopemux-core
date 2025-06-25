;; C++ Template Declarations
;; This file contains Tree-sitter queries for extracting C++ template constructs.
;; Follows conventions used in C and Python .scm files.

;; Template function definition
(template_declaration
  (function_definition) @template_function) @template

;; Template class definition
(template_declaration
  (class_specifier) @template_class) @template

;; TODO: Add support for template specializations and template aliases.
