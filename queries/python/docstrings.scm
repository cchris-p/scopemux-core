;; Module docstrings (at the top of a file)
(module
  (expression_statement
    (string) @module_docstring) ._*)

;; Class docstrings
(class_definition
  name: (identifier) @class_name
  body: (block
    (expression_statement
      (string) @class_docstring) ._*)) @class_with_docstring

;; Function docstrings
(function_definition
  name: (identifier) @function_name
  parameters: (parameters) @params
  body: (block
    (expression_statement
      (string) @function_docstring) ._*)) @function_with_docstring

;; Method docstrings (within class)
(class_definition
  body: (block
    (function_definition
      name: (identifier) @method_name
      parameters: (parameters) @params
      body: (block
        (expression_statement
          (string) @method_docstring) ._*)))) @method_with_docstring

;; Decorated function/method with docstring
(decorated_definition
  definition: (function_definition
    body: (block
      (expression_statement
        (string) @decorated_docstring) ._*))) @decorated_with_docstring

;; Multi-line strings that might be docstrings
(expression_statement
  (string
    (string_start) @string_start
    (string_content) @string_content
    (string_end) @string_end)) @possible_docstring
