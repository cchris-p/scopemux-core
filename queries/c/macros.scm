;; Object-like macros (simple constant definitions)
(preproc_def
  name: (identifier) @macro_name
  value: (preproc_arg)? @macro_value) @object_macro

;; Function-like macros (with parameters)
(preproc_function_def
  name: (identifier) @macro_name
  parameters: (preproc_params
    (identifier)* @param_name)
  value: (preproc_arg)? @macro_value) @function_macro

;; Conditional compilation directives
(preproc_ifdef
  name: (identifier) @condition_name) @ifdef_directive

(preproc_ifndef
  name: (identifier) @condition_name) @ifndef_directive

(preproc_if
  condition: (preproc_arg) @condition) @if_directive

(preproc_elif
  condition: (preproc_arg) @condition) @elif_directive

(preproc_else) @else_directive

(preproc_endif) @endif_directive

;; Undefine directives
(preproc_undef
  name: (identifier) @undef_name) @undef_directive

;; Line directives
(preproc_line
  (preproc_arg) @line_info) @line_directive

;; Error and warning directives
(preproc_error
  (preproc_arg)? @error_message) @error_directive

(preproc_warning
  (preproc_arg)? @warning_message) @warning_directive
