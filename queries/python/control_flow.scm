;; Python control-flow nodes. Keep these captures grammar-safe across current
;; Tree-sitter Python versions by avoiding fragile field-name dependencies.

;; If statements
(if_statement) @if_statement

;; Loops
(while_statement) @while_loop
(for_statement) @for_loop

;; With and try statements
(with_statement) @with_statement
(try_statement) @try_statement

;; Assertions and exits
(assert_statement) @assert
(return_statement) @return
(raise_statement) @raise

;; Break and continue
(break_statement) @break
(continue_statement) @continue

;; Match statements (Python 3.10+)
(match_statement) @match
