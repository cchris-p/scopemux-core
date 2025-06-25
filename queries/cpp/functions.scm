;; C++ Function Definitions and Declarations
;; This file contains Tree-sitter queries for extracting C++ function constructs.
;; Follows conventions used in C and Python .scm files.

;; Free function definition
(function_definition
  declarator: (function_declarator
    declarator: (identifier) @name
    parameters: (parameter_list) @params)
  body: (compound_statement) @body) @function

;; Free function declaration (prototype)
(declaration
  type: (_) @return_type
  declarator: (function_declarator
    declarator: (identifier) @name
    parameters: (parameter_list) @params)) @function_declaration

;; Member function definition (qualified name)
(function_definition
  declarator: (function_declarator
    declarator: (field_identifier) @member_name
    parameters: (parameter_list) @params)
  body: (compound_statement) @body) @member_function

;; TODO: Add support for template functions, operator overloads, and special member functions.
