;; TypeScript Enum Declarations
;; This file contains Tree-sitter queries for extracting TypeScript enum constructs.
;; Follows conventions used in other language .scm files.

;; Enum declaration
(enum_declaration
  name: (identifier) @name
  body: (enum_body) @body) @enum

;; TODO: Add support for const enums and ambient enums.
