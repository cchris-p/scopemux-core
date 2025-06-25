;; Class definitions with all relevant components
(class_definition
  name: (identifier) @name
  [
    (argument_list)  ; Base classes list
  ]?
  body: (block) @body) @node

;; Class with decorator - simplified to avoid attribute pattern issues
(decorated_definition
  (decorator) @decorator
  definition: (class_definition
    name: (identifier) @name
    body: (block) @body)) @node
