;; Simplified struct definition only
(struct_specifier) @struct

;; Forward declarations of structs
(declaration
  (struct_specifier
    (type_identifier) @name)) @struct_forward_declaration
