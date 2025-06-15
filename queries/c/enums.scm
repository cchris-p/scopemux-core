;; Enum definitions
(enum_specifier
  name: (type_identifier) @name
  body: (enumerator_list) @body) @enum

;; Anonymous enum
(enum_specifier
  body: (enumerator_list) @body) @anonymous_enum

;; Enum constants without values
(enumerator
  name: (identifier) @enumerator_name) @enum_constant

;; Enum constants with values
(enumerator
  name: (identifier) @enumerator_name
  value: (_) @enumerator_value) @enum_constant_with_value

;; Enum with typedef
(declaration
  type: (type_qualifier)? @type_qualifier
  declarator: (type_identifier) @typedef_name
  value: (enum_specifier
    name: (type_identifier)? @enum_name
    body: (enumerator_list) @body)) @typedef_enum

;; Forward declarations of enums
(declaration
  type: (enum_specifier
    name: (type_identifier) @name
    body: (_)?) @enum_forward_declaration)
