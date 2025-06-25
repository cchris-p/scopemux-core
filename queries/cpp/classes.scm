;; C++ Class and Struct Definitions
;; This file contains Tree-sitter queries for extracting C++ class and struct constructs.
;; Follows conventions used in C and Python .scm files.

;; Class definition
(class_specifier
  name: (type_identifier) @name
  body: (field_declaration_list) @body) @class

;; Struct definition
(struct_specifier
  name: (type_identifier) @name
  body: (field_declaration_list) @body) @struct

;; TODO: Add support for class inheritance, access specifiers, and template classes.
