;; If statements
(if_statement
  condition: (_) @condition
  consequence: (block) @consequence
  alternative: [(block) @alternative
                (if_statement) @elif]) @if_statement

;; While loops
(while_statement
  condition: (_) @condition
  body: (block) @body) @while_loop

;; For loops
(for_statement
  left: (_) @target
  right: (_) @iterator
  body: (block) @body) @for_loop

;; For loops with else
(for_statement
  left: (_) @target
  right: (_) @iterator
  body: (block) @body
  alternative: (else_clause
                 (block) @else_body)) @for_else_loop

;; While loops with else
(while_statement
  condition: (_) @condition
  body: (block) @body
  alternative: (else_clause
                 (block) @else_body)) @while_else_loop

;; With statements
(with_statement
  context_manager: (_) @context_manager
  body: (block) @body) @with_statement

;; Try-except statements
(try_statement
  body: (block) @try_body
  (except_clause
    type: (_)? @exception_type
    name: (_)? @exception_name
    body: (block) @except_body)* @except_clauses
  (else_clause
    body: (block) @else_body)?
  (finally_clause
    body: (block) @finally_body)?) @try_statement

;; Assert statements
(assert_statement
  condition: (_) @condition
  message: (_)? @message) @assert

;; Return statements
(return_statement
  value: (_)? @return_value) @return

;; Raise statements
(raise_statement
  exception: (_)? @exception
  cause: (_)? @cause) @raise

;; Break and continue
(break_statement) @break
(continue_statement) @continue

;; Match statements (Python 3.10+)
(match_statement
  subject: (_) @match_subject
  (case_clause
    pattern: (_) @match_pattern
    body: (block) @case_body)*) @match
