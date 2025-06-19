;; C++ Variable Declarations
;; This file contains Tree-sitter queries for extracting C++ variable declarations.
;; Follows conventions used in C and Python .scm files.

;; Global or local variable declaration
(declaration
  type: (_) @type
  declarator: (init_declarator
    declarator: (identifier) @name)) @variable

;; Member variable (field) declaration inside class/struct
(field_declaration
  type: (_) @type
  declarator: (field_identifier) @name) @member_variable

;; TODO: Add support for static, const, and thread_local variables.
