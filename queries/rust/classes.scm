;; Rust impl blocks, exposed as class-like containers.
;;
;; The implementing type (`impl Worker`, `impl Display for Point`, `impl<T>
;; Vec<T>`) becomes the container name so methods captured by methods.scm are
;; qualified by their type instead of colliding by simple name. Traits are
;; captured separately as interfaces; default trait methods attach there.
(impl_item
  type: (_) @name) @node
