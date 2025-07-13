;; Global variable declarations
(declaration
  (_) @variable_type
  (identifier) @name) @global_variable

;; Global variable with initializer
(declaration
  (_) @variable_type
  (init_declarator
    (identifier) @name
    (_) @value)) @variable

;; Variables with storage class specifiers (static, extern, etc.)
(declaration
  (storage_class_specifier) @storage_class
  (_) @variable_type
  [
    (identifier) @name
    (init_declarator
      (identifier) @name
      (_) @value)
  ]) @variable

;; Variables with type qualifiers (const, volatile)
(declaration
  (type_qualifier) @type_qualifier
  (_) @variable_type
  [
    (identifier) @name
    (init_declarator
      (identifier) @name
      (_) @value)
  ]) @variable

;; Array declarations
(declaration
  (_) @array_type
  (array_declarator
    (identifier) @name
    (_)? @array_size)) @variable

;; Array with initializer
(declaration
  (_) @array_type
  (init_declarator
    (array_declarator
      (identifier) @name
      (_)? @array_size)
    (_) @value)) @variable_with_init

;; Local variables (inside function body)
(compound_statement
  (declaration
    type: (_) @variable_type
    declarator: [
      (identifier) @name
      (init_declarator
        declarator: (identifier) @name
        value: (_) @value)
    ])) @local_variable

;; Pointer variables
(declaration
  type: (_) @pointer_base_type
  declarator: (pointer_declarator
    declarator: (identifier) @name)) @pointer_variable

;; Function parameters (which are also variables)
(parameter_declaration
  type: (_) @param_type
  declarator: (identifier) @param_name) @function_parameter
