;; C++ Enum Definitions
;; This file contains Tree-sitter queries for extracting C++ enum constructs.
;; Follows conventions used in C and Python .scm files.

;; Enum definition
(enum_specifier
  name: (type_identifier) @name
  body: (enumerator_list) @body) @enum

;; TODO: Add support for enum class and scoped enums.
