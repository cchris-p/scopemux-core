;; Regular union definition with name
(union_specifier
  (type_identifier) @name) @union

;; Anonymous union definition
(union_specifier) @union

;; Forward declarations of unions
(declaration
  (union_specifier
    (type_identifier) @name)) @union_forward_declaration
