;; Struct definitions
(struct_specifier
  name: (type_identifier) @name
  body: (field_declaration_list) @body) @struct

;; Struct with no name (anonymous struct)
(struct_specifier
  body: (field_declaration_list) @body) @anonymous_struct

;; Struct field declarations
(field_declaration
  type: (_) @field_type
  declarator: (field_identifier) @field_name) @struct_field

;; Struct with typedef
(declaration
  type: (type_qualifier)? @type_qualifier
  declarator: (type_identifier) @typedef_name
  value: (struct_specifier
    name: (type_identifier)? @struct_name
    body: (field_declaration_list) @body)) @typedef_struct

;; Forward declarations of structs
(declaration
  type: (struct_specifier
    name: (type_identifier) @name
    body: (_)?) @struct_forward_declaration)
