;; Basic function with name, parameters, and body
(function_definition
  name: (identifier) @name
  parameters: (parameters) @params
  [
    return_type: (type) @return_type
  ]?
  body: (block) @body) @node

;; Function with docstring
(function_definition
  name: (identifier) @name
  parameters: (parameters) @params
  body: (block
    (expression_statement
      (string) @docstring) ._*)) @node

;; Decorated function - simplified to avoid attribute pattern issues
(decorated_definition
  (decorator) @decorator
  definition: (function_definition
    name: (identifier) @name
    parameters: (parameters) @params
    body: (block) @body)) @node
