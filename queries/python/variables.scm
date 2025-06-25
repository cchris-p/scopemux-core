;; Basic variable declarations - simplified patterns

;; Simple assignment
(assignment
  left: (identifier) @name
  right: (_) @value) @node

;; Class attribute
(class_definition
  body: (block
    (expression_statement
      (assignment
        left: (identifier) @name
        right: (_) @value)))) @node

;; Function local variable
(function_definition
  body: (block
    (expression_statement
      (assignment
        left: (identifier) @name
        right: (_) @value)))) @node
