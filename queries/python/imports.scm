;; Regular import statements
(import_statement
  name: [
    (dotted_name
      (identifier) @module_name)
    (identifier) @module_name
  ]) @import

;; Import with alias
(import_statement
  name: [
    (dotted_name
      (identifier) @module_name)
    (identifier) @module_name
  ]
  alias: (identifier) @alias) @aliased_import

;; Import from
(import_from_statement
  module_name: [
    (dotted_name
      (identifier) @source_module)
    (identifier) @source_module
  ]
  name: [
    (dotted_name
      (identifier) @imported_name)
    (identifier) @imported_name
  ]) @from_import

;; Import from with alias
(import_from_statement
  module_name: [
    (dotted_name
      (identifier) @source_module)
    (identifier) @source_module
  ]
  name: [
    (dotted_name
      (identifier) @imported_name)
    (identifier) @imported_name
  ]
  alias: (identifier) @alias) @from_import_with_alias

;; Import multiple items
(import_from_statement
  module_name: [
    (dotted_name
      (identifier) @source_module)
    (identifier) @source_module
  ]
  name: (wildcard_import) @wildcard) @wildcard_import

;; Import from with multiple names
(import_from_statement
  module_name: [
    (dotted_name
      (identifier) @source_module)
    (identifier) @source_module
  ]
  name: (import_from_names
    (identifier) @imported_name)) @from_import_multiple
