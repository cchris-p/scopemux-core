;; Function definitions
(function_definition
  declarator: (function_declarator
    declarator: (identifier) @name
    parameters: (parameter_list) @params)
  body: (compound_statement) @body) @function

;; Function declarations (prototypes)
(declaration
  type: (_) @return_type
  declarator: (function_declarator
    declarator: (identifier) @name
    parameters: (parameter_list) @params)) @function_declaration

;; Function with storage class specifier (static, extern, etc.)
(declaration
  (storage_class_specifier) @storage_class
  type: (_) @return_type
  declarator: (function_declarator
    declarator: (identifier) @name
    parameters: (parameter_list) @params)) @function_with_storage_class

;; Function with type qualifiers (const, volatile)
(function_definition
  type_qualifier: (_) @type_qualifier
  declarator: (function_declarator
    declarator: (identifier) @name
    parameters: (parameter_list) @params)
  body: (compound_statement) @body) @function_with_qualifier

;; Function parameters
(parameter_declaration
  type: (_) @param_type
  declarator: (identifier) @param_name) @parameter
