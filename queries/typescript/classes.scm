;; TypeScript Class and Interface Declarations
;; This file contains Tree-sitter queries for extracting TypeScript class and interface constructs.
;; Follows conventions used in other language .scm files.

;; Class declaration
(class_declaration
  name: (identifier) @name
  body: (class_body) @body) @class

;; Interface declaration
(interface_declaration
  name: (identifier) @name
  body: (object_type) @body) @interface

;; TODO: Add support for class heritage (extends/implements) and abstract classes.
