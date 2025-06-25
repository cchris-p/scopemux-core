;; TypeScript Interface Declarations
;; This file contains Tree-sitter queries for extracting TypeScript interface constructs.
;; Follows conventions used in other language .scm files.

;; Interface declaration
(interface_declaration
  name: (identifier) @name
  body: (object_type) @body) @interface

;; TODO: Add support for interface heritage (extends).
