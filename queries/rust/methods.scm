;; Rust methods (functions inside impl blocks)
(impl_item
  body: (declaration_list
    (function_item
      name: (identifier) @name
      parameters: (parameters) @params
      body: (block) @body) @method))
