;; Global variable declarations
(declaration
  type: (_) @variable_type
  declarator: (identifier) @name) @global_variable

;; Global variable with initializer
(declaration
  type: (_) @variable_type
  declarator: (init_declarator
    declarator: (identifier) @name
    value: (_) @value)) @global_variable_with_init

;; Variables with storage class specifiers (static, extern, etc.)
(declaration
  (storage_class_specifier) @storage_class
  type: (_) @variable_type
  declarator: [
    (identifier) @name
    (init_declarator
      declarator: (identifier) @name
      value: (_) @value)
  ]) @variable_with_storage_class

;; Variables with type qualifiers (const, volatile)
(declaration
  (type_qualifier) @type_qualifier
  type: (_) @variable_type
  declarator: [
    (identifier) @name
    (init_declarator
      declarator: (identifier) @name
      value: (_) @value)
  ]) @variable_with_type_qualifier

;; Array declarations
(declaration
  type: (_) @array_type
  declarator: (array_declarator
    declarator: (identifier) @name
    size: (_)? @array_size)) @array_variable

;; Array with initializer
(declaration
  type: (_) @array_type
  declarator: (init_declarator
    declarator: (array_declarator
      declarator: (identifier) @name
      size: (_)? @array_size)
    value: (_) @value)) @array_variable_with_init

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
