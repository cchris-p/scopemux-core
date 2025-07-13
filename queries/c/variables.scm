;; Variable declarations (global or local, no initializer)
(declaration
  declarator: (identifier) @name
  type: (_) @variable_type) @variable

;; Field-anchored fallback for variable declarations
(declaration
  declarator: (init_declarator
    declarator: (identifier) @name)
  type: (_) @variable_type) @variable

;; Structure-based fallback for variable declarations
(declaration
  (identifier) @name
  (_) @variable_type) @variable

;; Variable declarations with initializer
(declaration
  declarator: (init_declarator
    declarator: (identifier) @name
    value: (_) @value)
  type: (_) @variable_type) @variable_with_init

;; Field-anchored fallback for variable declarations with initializer
(declaration
  declarator: (init_declarator
    declarator: (identifier) @name
    value: (_))
  type: (_) @variable_type) @variable_with_init

;; Structure-based fallback for variable declarations with initializer
(declaration
  (identifier) @name
  (_) @value
  (_) @variable_type) @variable_with_init

;; Pointer variable declarations
(declaration
  declarator: (pointer_declarator
    declarator: (identifier) @name)
  type: (_) @pointer_base_type) @pointer_variable

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
