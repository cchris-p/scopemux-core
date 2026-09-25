;; Rust call sites and macro invocations, extracted as call references.
;;
;; The call expression/macro invocation is the primary node (@node) and is
;; mapped to NODE_IDENTIFIER so it participates in reference resolution and
;; call-graph edges without being registered as a declared symbol. @name is the
;; callee path or macro name used for resolution.
(call_expression
  function: [
    (identifier) @name
    (scoped_identifier) @name
    (field_expression field: (field_identifier) @name)
  ]) @node

(macro_invocation
  macro: [
    (identifier) @name
    (scoped_identifier) @name
  ]) @node
