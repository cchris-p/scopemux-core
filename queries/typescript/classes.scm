;; TypeScript Class Declarations
;; This file contains Tree-sitter queries for extracting TypeScript class constructs.
;; Follows conventions used in other language .scm files.

;; Class declaration
(class_declaration
  name: (type_identifier) @name
  body: (class_body) @body) @class

;; TODO: Add support for class heritage (extends/implements) and abstract classes.
