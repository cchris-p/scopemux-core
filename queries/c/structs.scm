;; Named struct definition (not typedef)
(declaration
  type: (struct_specifier
          name: (type_identifier) @name
        ) @struct
)

;; Typedef struct (anonymous or named)
(type_definition
  type: (struct_specifier) @struct
  declarator: (type_identifier) @name
)
