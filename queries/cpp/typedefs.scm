;; C++ Typedefs and Using Aliases
;; This file contains Tree-sitter queries for extracting C++ typedef and using constructs.
;; Follows conventions used in C and Python .scm files.

;; Typedef declaration
(type_definition
  name: (type_identifier) @name
  type: (_) @type) @typedef

;; Using alias declaration
(using_declaration
  name: (type_identifier) @name
  type: (_) @type) @using_alias

;; TODO: Add support for template aliases.
