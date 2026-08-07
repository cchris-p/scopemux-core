;; C imports - Tree-sitter patterns for #include directives

;; System header includes
(preproc_include
  path: (system_lib_string) @name) @include

;; Local includes with "" quotes
(preproc_include
  path: (string_literal) @name) @include