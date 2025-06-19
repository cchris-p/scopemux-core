;; JavaScript Class Declarations
;; This file contains Tree-sitter queries for extracting JavaScript class constructs.
;; Follows conventions used in other language .scm files.

;; Class declaration
(class_declaration
  name: (identifier) @name
  body: (class_body) @body) @class

;; TODO: Add support for class heritage (extends).
