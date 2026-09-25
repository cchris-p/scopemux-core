;; Rust methods: functions inside inherent/trait impl blocks and trait
;; definitions (default methods and signatures). The enclosing impl block is
;; captured by classes.scm and the enclosing trait by interfaces.scm, so the
;; method node is scoped by its implementing type or trait.
(impl_item
  body: (declaration_list
    (function_item
      name: (identifier) @name
      parameters: (parameters) @params
      body: (block) @body) @method))

(trait_item
  body: (declaration_list
    (function_item
      name: (identifier) @name
      parameters: (parameters) @params) @method))
