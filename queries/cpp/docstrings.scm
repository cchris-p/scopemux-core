;; C++ docstrings - supports both multi-line /** ... */ style comments and // style comments

;; File-level docstring (at the start of the file)
(comment) @file_docstring @docstring

;; Function docstrings (immediately before a function_definition)
(comment) @function_docstring @docstring

;; Class docstrings (immediately before a class_definition)
(comment) @class_docstring @docstring

;; Structure docstrings (immediately before a structure)
(comment) @struct_docstring @docstring

;; Method docstrings (immediately before a method_definition)
(comment) @method_docstring @docstring

;; Template docstrings (immediately before a template_declaration)
(comment) @template_docstring @docstring

;; Namespace docstrings (immediately before a namespace_definition)
(comment) @namespace_docstring @docstring
