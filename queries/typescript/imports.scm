;; TypeScript Import Statements
;; This file contains Tree-sitter queries for extracting TypeScript import statements.
;; Follows conventions used in other language .scm files.

;; Import statement (ES6)
(import_statement
  source: (string) @source) @import

;; TODO: Add support for named, default, and namespace imports.
