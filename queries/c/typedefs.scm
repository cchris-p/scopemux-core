;; Basic typedefs
(type_definition
  name: (type_identifier) @typedef_name
  type: (_) @original_type) @typedef

;; Struct typedefs
(type_definition
  name: (type_identifier) @typedef_name
  type: (struct_specifier
    name: (type_identifier)? @struct_name
    body: (field_declaration_list) @body)) @struct_typedef

;; Union typedefs
(type_definition
  name: (type_identifier) @typedef_name
  type: (union_specifier
    name: (type_identifier)? @union_name
    body: (field_declaration_list) @body)) @union_typedef

;; Enum typedefs
(type_definition
  name: (type_identifier) @typedef_name
  type: (enum_specifier
    name: (type_identifier)? @enum_name
    body: (enumerator_list) @body)) @enum_typedef

;; Function pointer typedefs
(type_definition
  name: (type_identifier) @typedef_name
  type: (function_declarator
    declarator: (pointer_declarator)
    parameters: (parameter_list) @params)) @function_pointer_typedef

;; Array typedefs
(type_definition
  name: (type_identifier) @typedef_name
  type: (array_declarator
    element: (_) @element_type
    size: (_)? @array_size)) @array_typedef

;; Pointer typedefs
(type_definition
  name: (type_identifier) @typedef_name
  type: (pointer_declarator
    declarator: (_) @pointed_type)) @pointer_typedef
