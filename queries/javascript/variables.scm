;; JavaScript Variable Declarations
;; This file contains Tree-sitter queries for extracting JavaScript variable declarations.
;; Follows conventions used in other language .scm files.

;; Variable declaration (let, const, var)
(lexical_declaration
  (variable_declarator
    name: (identifier) @name
    value: (_) @value?)) @variable

;; TODO: Add support for destructuring.
