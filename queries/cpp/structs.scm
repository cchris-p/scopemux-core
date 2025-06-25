;; C++ struct definitions

;; Basic struct declaration
(struct_specifier
  name: (type_identifier) @struct_name) @struct_definition

;; Struct with members
(struct_specifier
  name: (type_identifier) @struct_name
  body: (field_declaration_list)) @struct_with_fields

;; Template struct (simplified format)
(template_declaration
  (struct_specifier
    name: (type_identifier) @template_struct_name)) @template_struct_definition
