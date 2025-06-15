;; Basic function with name, parameters, and body
(function_definition
  name: (identifier) @name
  parameters: (parameters) @params
  [
    return_type: (type) @return_type
  ]?
  body: (block) @body) @function

;; Function with docstring
(function_definition
  name: (identifier) @name
  parameters: (parameters) @params
  body: (block
    (expression_statement
      (string) @docstring) ._*)) @function_with_docstring

;; Decorated function
(decorated_definition
  (decorator
    "@" @decorator_symbol
    name: [
      (identifier) @decorator
      (attribute 
        object: (identifier)? @decorator_object
        attribute: (identifier) @decorator_attribute)
    ]) 
  definition: (function_definition) @decorated_function) @function_with_decorator
