;; Simplified variable extraction for C to avoid duplicates

;; Match variable declarations (covers most cases without overlap)
(declaration
  declarator: [
    (identifier) @name
    (init_declarator declarator: (identifier) @name)
    (pointer_declarator declarator: (identifier) @name)
    (init_declarator declarator: (pointer_declarator declarator: (identifier) @name))
    (array_declarator declarator: (identifier) @name)
    (init_declarator declarator: (array_declarator declarator: (identifier) @name))
  ]
) @variable

;; Function parameters
(parameter_declaration
  declarator: [
    (identifier) @name
    (pointer_declarator declarator: (identifier) @name)
    (array_declarator declarator: (identifier) @name)
  ]
) @variable
