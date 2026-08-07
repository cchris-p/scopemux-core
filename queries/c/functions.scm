;; Function definitions (direct, pointer, parenthesized)
;; Minimal compiling pattern for function definitions
;; (Backed by vendor/tree-sitter-c/grammar.js and node-types.json)

;; Canonical function name extraction (one layer of nesting)
;; (function_definition declarator: (function_declarator declarator: (identifier) @name)) @function
(function_definition
  declarator: (function_declarator
    declarator: (identifier) @name
  )
) @function

;; Pointer declarator nesting
(function_definition
  declarator: (function_declarator
    declarator: (pointer_declarator
      declarator: (identifier) @name
    )
  )
) @function

