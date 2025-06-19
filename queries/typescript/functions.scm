;; TypeScript Function Declarations and Expressions
;; This file contains Tree-sitter queries for extracting TypeScript function constructs.
;; Follows conventions used in other language .scm files.

;; Function declaration
(function_declaration
  name: (identifier) @name
  parameters: (formal_parameters) @params
  body: (statement_block) @body) @function

;; Arrow function expression
(lexical_declaration
  (variable_declarator
    name: (identifier) @name
    value: (arrow_function
      parameters: (formal_parameters) @params
      body: (_) @body))) @arrow_function

;; Function expression assigned to variable
(lexical_declaration
  (variable_declarator
    name: (identifier) @name
    value: (function
      parameters: (formal_parameters) @params
      body: (statement_block) @body))) @function_expression

;; TODO: Add support for async, generator, and overloaded functions.
