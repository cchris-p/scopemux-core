;; CASCADE_DEBUG_MARKER

;; Capture function definition with signature
(function_definition
  declarator: (function_declarator
    declarator: (identifier) @name
    parameters: (parameter_list)?) @signature
  type: (_) @return_type) @function

;; Capture docstring (comment before function)
((comment) @docstring
 (#select-adjacent-before 1 @function))
