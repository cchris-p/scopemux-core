;; Method definitions within classes
(class_definition
  body: (block 
    (function_definition
      name: (identifier) @name
      parameters: (parameters) @params
      [
        return_type: (type) @return_type
      ]?
      body: (block) @body) @method))

;; Decorated methods
(class_definition
  body: (block
    (decorated_definition
      (decorator
        "@" @decorator_symbol
        name: [
          (identifier) @decorator
          (attribute 
            object: (identifier)? @decorator_object
            attribute: (identifier) @decorator_attribute)
        ]) 
      definition: (function_definition) @decorated_method) @method_with_decorator))

;; Class methods with docstring
(class_definition
  body: (block
    (function_definition
      name: (identifier) @name
      parameters: (parameters) @params
      body: (block
        (expression_statement
          (string) @docstring) ._*)) @method_with_docstring))
