;; Union definitions
(union_specifier
  name: (type_identifier) @name
  body: (field_declaration_list) @body) @union

;; Anonymous union
(union_specifier
  body: (field_declaration_list) @body) @anonymous_union

;; Union field declarations
(field_declaration
  type: (_) @field_type
  declarator: (field_identifier) @field_name) @union_field

;; Union with typedef
(declaration
  type: (type_qualifier)? @type_qualifier
  declarator: (type_identifier) @typedef_name
  value: (union_specifier
    name: (type_identifier)? @union_name
    body: (field_declaration_list) @body)) @typedef_union

;; Forward declarations of unions
(declaration
  type: (union_specifier
    name: (type_identifier) @name
    body: (_)?) @union_forward_declaration)
