;; Global variable declarations
(module
  (assignment
    left: [
      (identifier) @name
      (pattern_list
        (identifier) @name)
    ]
    right: (_) @value) @global_variable)

;; Typed global variables
(module
  (assignment
    left: (typed_parameter
      (identifier) @name
      (type) @type)
    right: (_) @value) @typed_global_variable)

;; Class attributes
(class_definition
  body: (block
    (expression_statement
      (assignment
        left: [
          (identifier) @name
          (pattern_list
            (identifier) @name)
        ]
        right: (_) @value)) @class_attribute))

;; Typed class attributes
(class_definition
  body: (block
    (expression_statement
      (assignment
        left: (typed_parameter
          (identifier) @name
          (type) @type)
        right: (_) @value)) @typed_class_attribute))

;; Function local variables
(function_definition
  body: (block
    (expression_statement
      (assignment
        left: [
          (identifier) @name
          (pattern_list
            (identifier) @name)
        ]
        right: (_) @value)) @local_variable))

;; Typed local variables
(function_definition
  body: (block
    (expression_statement
      (assignment
        left: (typed_parameter
          (identifier) @name
          (type) @type)
        right: (_) @value)) @typed_local_variable))

;; For loop variables
(for_statement
  left: [
    (identifier) @name
    (pattern_list
      (identifier) @name)
  ]) @for_variable

;; Multiple assignment (tuple unpacking)
(assignment
  left: (pattern_list
    (identifier) @name) @names
  right: (_) @value) @multiple_assignment
