;; JavaScript Method Declarations

;; Class method definition
(class_declaration
  body: (class_body
    (method_definition
      name: (property_identifier) @name
      parameters: (formal_parameters) @params
      body: (statement_block) @body))) @node

;; Standalone method definition
(method_definition
  name: (property_identifier) @name
  parameters: (formal_parameters) @params
  body: (statement_block) @body) @node
