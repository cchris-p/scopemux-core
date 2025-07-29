;; Variable declarations - simplified working version
;; Based on Tree-sitter C grammar patterns

;; Simple variable declaration
(declaration
  declarator: (identifier) @name
) @variable

;; Variable declaration with initializer
(declaration
  declarator: (init_declarator
    declarator: (identifier) @name
    value: (_)
  )
) @variable

;; Pointer variable declaration
(declaration
  declarator: (pointer_declarator
    declarator: (identifier) @name
  )
) @variable

;; Pointer variable with initializer
(declaration
  declarator: (init_declarator
    declarator: (pointer_declarator
      declarator: (identifier) @name
    )
    value: (_)
  )
) @variable

;; Function parameter
(parameter_declaration
  declarator: (identifier) @name
) @variable
