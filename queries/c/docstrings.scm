;; C docstrings - Tree-sitter patterns for C documentation comments

;; Capture function docstrings (preceding function definitions)
(translation_unit
  (comment) @docstring
  . ;; Sibling relationship (comment immediately followed by function)
  (function_definition)
  (#match? @docstring "^/\*\*")
)

;; Capture struct docstrings (preceding struct definitions)
(translation_unit
  (comment) @docstring
  . ;; Sibling relationship (comment immediately followed by struct)
  (struct_specifier)
  (#match? @docstring "^/\*\*")
)

;; Capture enum docstrings
(translation_unit
  (comment) @docstring
  . ;; Sibling relationship
  (enum_specifier)
  (#match? @docstring "^/\*\*")
)

;; Capture file-level docstrings (usually at the top of file)
(translation_unit
  (comment) @docstring
  (#match? @docstring "^/\*\*")
)
