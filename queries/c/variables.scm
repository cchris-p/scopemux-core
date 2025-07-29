;; Robust variable extraction for C
;; Covers: scalars, pointers, arrays, multiple declarators, function params

;; Match all identifiers in comma-separated variable declarations
(declaration
  (init_declarator
    declarator: (identifier) @name
    value: (_)?
  )+
) @variable

;; Single variable declaration (no initializer)
(declaration
  declarator: (identifier) @name
) @variable

;; Single variable declaration with initializer
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

;; Array variable declaration
(declaration
  declarator: (array_declarator
    declarator: (identifier) @name
    size: (_)?
  )
) @variable

;; Array variable with initializer
(declaration
  declarator: (init_declarator
    declarator: (array_declarator
      declarator: (identifier) @name
      size: (_)?
    )
    value: (_)
  )
) @variable

;; Pointer to array variable
(declaration
  declarator: (pointer_declarator
    declarator: (array_declarator
      declarator: (identifier) @name
      size: (_)?
    )
  )
) @variable

;; Function parameter (identifier)
(parameter_declaration
  declarator: (identifier) @name
) @variable

;; Function parameter (pointer)
(parameter_declaration
  declarator: (pointer_declarator
    declarator: (identifier) @name
  )
) @variable

;; Function parameter (array)
(parameter_declaration
  declarator: (array_declarator
    declarator: (identifier) @name
    size: (_)?
  )
) @variable
