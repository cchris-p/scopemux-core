;; System header includes
(preproc_include
  path: (system_lib_string) @system_header_path) @system_include

;; Local header includes
(preproc_include
  path: (string_literal) @local_header_path) @local_include

;; Include with line continuation
(preproc_include
  (preproc_arg) @include_with_continuation) @include_continuation

;; Include guards
(preproc_ifdef
  name: (identifier) @guard_macro
  (translation_unit) @guarded_block) @include_guard

;; Include-once pragma
(preproc_pragma
  (preproc_arg) @pragma_once) @pragma_include_guard
