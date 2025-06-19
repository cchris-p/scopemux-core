;; JavaScript Method Declarations
;; This file contains Tree-sitter queries for extracting JavaScript class and object methods.
;; Follows conventions used in other language .scm files.

;; Class method definition
(method_definition
  name: (property_identifier) @name
  parameters: (formal_parameters) @params
  body: (statement_block) @body) @method

;; Object method (shorthand in object literal)
(pair
  key: (property_identifier) @name
  value: (function
    parameters: (formal_parameters) @params
    body: (statement_block) @body)) @object_method

;; TODO: Add support for static, async, and generator methods.
