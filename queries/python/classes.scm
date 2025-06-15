;; Class definitions with all relevant components
(class_definition
  name: (identifier) @name
  [
    (argument_list
      (identifier) @superclass)  ; Capture base classes
    (argument_list)
  ]?
  body: (block) @body) @class

;; Class with decorator
(decorated_definition
  (decorator
    "@" @decorator_symbol
    name: [
      (identifier) @decorator
      (attribute 
        object: (identifier)? @decorator_object
        attribute: (identifier) @decorator_attribute)
    ]) 
  definition: (class_definition) @decorated_class) @class_with_decorator
