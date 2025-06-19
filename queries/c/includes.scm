;; System header includes
(preproc_include
  (system_lib_string) @system_header_path) @system_include

;; System includes with <> brackets
(preproc_include
  path: (system_lib_string) @name) @include

;; Local includes with "" quotes
(preproc_include
  path: (string_literal) @name) @include

;; Generic include fallback
(preproc_include) @include
