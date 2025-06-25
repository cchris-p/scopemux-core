;; C++ Include Directives
;; This file contains Tree-sitter queries for extracting C++ #include directives.
;; Follows conventions used in C and Python .scm files.

;; Include directive
(preproc_include
  path: (string) @path) @include

;; TODO: Add support for angle-bracket includes and macro-based includes.
