;; Python Method Declarations

;; Simple method pattern
(class_definition
  name: (identifier) @class_name
  body: (block
    (function_definition
      name: (identifier) @name
      parameters: (parameters) @params
      body: (block)) @body)) @node

;; Method with any parameters
(class_definition
  body: (block
    (function_definition
      name: (identifier) @name
      parameters: (parameters)) @method)) @node
