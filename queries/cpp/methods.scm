;; C++ Member Function (Method) Definitions
;; This file contains Tree-sitter queries for extracting C++ class/struct member functions.
;; Follows conventions used in C and Python .scm files.

;; Member function definition (inside class)
(function_definition
  declarator: (function_declarator
    declarator: (field_identifier) @name
    parameters: (parameter_list) @params)
  body: (compound_statement) @body) @method

;; Member function declaration (prototype inside class)
(field_declaration
  declarator: (function_declarator
    declarator: (field_identifier) @name
    parameters: (parameter_list) @params)) @method_declaration

;; TODO: Add support for const, static, virtual, and override specifiers.
