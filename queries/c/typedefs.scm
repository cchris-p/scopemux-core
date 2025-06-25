;; Typedef with name
(type_definition
  declarator: (type_identifier) @name) @typedef

;; Generic typedef fallback
(type_definition) @typedef

;; Pointer typedefs
(type_definition
  (type_identifier) @typedef_name
  (pointer_declarator
    (_) @pointed_type)) @pointer_typedef
