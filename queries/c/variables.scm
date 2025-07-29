;; Variable declarations (global or local, no initializer)
;; Minimal variable declaration extraction for C

;; Match all declarators in a declaration with multiple declarators
(declaration
  declarator: (identifier) @name) @variable

(declaration
  declarator: (init_declarator
    declarator: (identifier) @name
    value: (_))
) @variable_with_init

(declaration
  declarator: (pointer_declarator
    declarator: (identifier) @name)
) @pointer_variable

(declaration
  declarator: (init_declarator
    declarator: (pointer_declarator
      declarator: (identifier) @name)
    value: (_))
) @pointer_variable_with_init

;; Match declarators in comma-separated variable declarations
(declaration
  declarator: (identifier) @name
) @variable

(declaration
  declarator: (init_declarator
    declarator: (identifier) @name
  )
) @variable

;; Variable declaration with initializer
(declaration
  declarator: (init_declarator
    declarator: (identifier) @name
    value: (_))
) @variable_with_init

;; Pointer variable declaration
(declaration
  declarator: (pointer_declarator
    declarator: (identifier) @name)
) @pointer_variable

;; Pointer variable declaration with initializer
(declaration
  declarator: (init_declarator
    declarator: (pointer_declarator
      declarator: (identifier) @name)
    value: (_))
) @pointer_variable_with_init

;; Field-anchored fallback for pointer variable declarations
(declaration
  declarator: (pointer_declarator
    declarator: (identifier) @name)
  type: (_) @pointer_base_type) @pointer_variable

;; Structure-based fallback for pointer variable declarations
(declaration
  (identifier) @name
  (_) @pointer_base_type) @pointer_variable

;; Function parameter variables
(parameter_declaration
  declarator: (identifier) @param_name
  type: (_) @param_type) @function_parameter

;; Field-anchored fallback for function parameter variables
(parameter_declaration
  declarator: (identifier) @param_name
  type: (_) @param_type) @function_parameter

;; Structure-based fallback for function parameter variables
(parameter_declaration
  (identifier) @param_name
  (_) @param_type) @function_parameter
