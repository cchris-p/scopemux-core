# ScopeMux Core (`scopemux_core`) Python API Reference

This document details the Python API for the `scopemux_core` module, which is a C extension providing core functionalities for ScopeMux.

## Module Name

`scopemux_core`

## Top-Level Exports

### Attributes

*   `__doc__` (str): Module documentation string: "ScopeMux C bindings\n\nThis module provides high-performance C implementations of the ScopeMux\ncore functionality, including parsing, IR generation, and context management."
*   `__version__` (str): Version of the module, e.g., "0.1.0".
*   `DEFAULT_TOKEN_BUDGET` (int): Default token budget, e.g., 2048.

### Functions

*   `detect_language(filename: str, content: str = None) -> int`
    *   Detects the programming language from a filename and optional file content.
    *   Returns one of the `LANG_*` constants.

### Constants

#### Language Constants (`LANG_*`)

Used to specify or identify programming languages.

*   `LANG_UNKNOWN`
*   `LANG_C`
*   `LANG_CPP`
*   `LANG_PYTHON`
*   `LANG_JAVASCRIPT`
*   `LANG_TYPESCRIPT`
*   `LANG_RUST`

#### Node Type Constants (`NODE_*`)

Used to identify the type of an IR (Intermediate Representation) node.

*   `NODE_UNKNOWN`
*   `NODE_FUNCTION`
*   `NODE_METHOD`
*   `NODE_CLASS`
*   `NODE_STRUCT`
*   `NODE_ENUM`
*   `NODE_INTERFACE`
*   `NODE_NAMESPACE`
*   `NODE_MODULE`
*   `NODE_COMMENT`
*   `NODE_DOCSTRING`

#### Compression Level Constants (`COMPRESSION_*`)

Used to specify or identify the level of compression applied to an information block.

*   `COMPRESSION_NONE`
*   `COMPRESSION_LIGHT`
*   `COMPRESSION_MEDIUM`
*   `COMPRESSION_HEAVY`
*   `COMPRESSION_SIGNATURE_ONLY`

## Exported Python Types (Classes)

### 1. `scopemux_core.ParserContext`

Manages the parsing process and stores the resulting Intermediate Representation (IR).

*   **Constructor**: `ParserContext()`
    *   Initializes a new parser context.

*   **Methods**:
    *   `parse_file(filename: str, language: int = LANG_UNKNOWN)`
        *   Parses the content of the specified file.
        *   `language`: Optional; one of the `LANG_*` constants. If `LANG_UNKNOWN` or not provided, the language is detected from the filename.
    *   `parse_string(content: str, filename: str = None, language: int = LANG_UNKNOWN)`
        *   Parses the given string content.
        *   `filename`: Optional; used for context and language detection if `language` is `LANG_UNKNOWN`.
        *   `language`: Optional; one of the `LANG_*` constants.
    *   `get_node(qualified_name: str) -> ASTNode | None`
        *   Retrieves an `ASTNode` by its fully qualified name.
        *   Returns the `ASTNode` object if found, otherwise `None`.
    *   `get_nodes_by_type(node_type: int) -> list[ASTNode]`
        *   Retrieves all `ASTNode` objects of a specific type.
        *   `node_type`: One of the `NODE_*` constants.
        *   Returns a list of `ASTNode` objects.

### 2. `scopemux_core.ASTNode`

Represents a node in the Abstract Syntax Tree (e.g., a function, class, comment).
Instances are typically obtained from `ParserContext` methods or Tree-sitter queries, not constructed directly in Python.

*   **Properties** (read-only):
    *   `name` (str): The simple name of the node (e.g., function name, class name).
    *   `qualified_name` (str): The fully qualified name of the node (e.g., `namespace.class.method`).
    *   `signature` (str): The signature of the node, if applicable (e.g., function signature).
    *   `docstring` (str): The docstring or documentation associated with the node.
    *   `content` (str): The raw source code content of the node itself.
    *   `type` (int): The type of the node, represented by one of the `NODE_*` constants.
    *   `range` (dict): A dictionary representing the node's start and end position in the source code.
        *   Example: `{'start': {'line': int, 'column': int, 'offset': int}, 'end': {'line': int, 'column': int, 'offset': int}}`

### 3. `scopemux_core.ContextEngine`

Manages information blocks, ranks them by relevance, and compresses them to fit a token budget.

*   **Constructor**: `ContextEngine(options: dict = None)`
    *   Initializes a new context engine.
    *   `options` (dict, optional): A dictionary to configure the engine. Keys include:
        *   `max_tokens` (int, default: 2048): The target maximum number of tokens for the compressed context.
        *   `recency_weight` (float, default: 1.0): Weight for how recently a block was accessed or focused.
        *   `proximity_weight` (float, default: 1.0): Weight for proximity to the cursor.
        *   `similarity_weight` (float, default: 1.0): Weight for semantic similarity to a query.
        *   `reference_weight` (float, default: 1.0): Weight for how often a block is referenced.
        *   `user_focus_weight` (float, default: 2.0): Weight for blocks explicitly focused by the user.
        *   `preserve_structure` (bool, default: True): Whether to try to preserve structural elements (like class/function definitions) during compression.
        *   `prioritize_functions` (bool, default: True): Whether to give higher priority to function/method blocks.

*   **Methods**:
    *   `add_parser_context(parser_ctx: ParserContext) -> int`
        *   Adds all IR nodes from the given `ParserContext` to the engine as information blocks.
        *   Returns the number of information blocks successfully added.
    *   `rank_blocks(cursor_file: str, cursor_line: int, cursor_column: int, query: str = None)`
        *   Ranks all managed information blocks based on relevance to the cursor position and optional query.
    *   `compress()`
        *   Applies compression strategies to the information blocks to meet the `max_tokens` budget, based on their ranks and compression levels.
    *   `get_context() -> str`
        *   Constructs and returns the final compressed context as a single string, concatenating the content of selected and compressed information blocks.
    *   `estimate_tokens(text: str) -> int`
        *   Estimates the number of tokens the given text would consume.
    *   `update_focus(node_qualified_names: list[str], focus_value: float) -> int`
        *   Updates the user focus score for the specified information blocks (identified by their IR node qualified names).
        *   Returns the number of blocks whose focus was updated.
    *   `reset_compression()`
        *   Resets the compression level of all information blocks to `COMPRESSION_NONE`.

### 4. `scopemux_core.InfoBlock`

Represents a unit of code or documentation managed by the `ContextEngine`. Instances are managed internally by the `ContextEngine`.

*   **Properties** (read-only):
    *   `original_tokens` (int): The number of tokens in the original, uncompressed content of the block.
    *   `compressed_tokens` (int): The number of tokens in the current (possibly compressed) content of the block.
    *   `compression_level` (int): The current compression level applied to the block, represented by one of the `COMPRESSION_*` constants.
    *   `relevance` (dict): A dictionary containing various relevance scores for the block.
        *   Keys: `recency`, `cursor_proximity`, `semantic_similarity`, `reference_count`, `user_focus` (all floats).
    *   `compressed_content` (str): The current (possibly compressed) textual content of the block.
    *   `ast_node` (`ASTNode`): A reference to the original `ASTNode` from which this information block was derived.

### 5. `scopemux_core.TreeSitterParser`

Provides an interface to Tree-sitter parsers for specific languages and utilities to convert Tree-sitter syntax trees into ScopeMux IR.

*   **Constructor**: `TreeSitterParser(language: int)`
    *   Initializes a Tree-sitter parser for the specified language.
    *   `language`: One of the `LANG_*` constants.

*   **Methods**:
    *   `parse_string(content: str) -> PyCapsule`
        *   Parses the given string content using the initialized Tree-sitter parser.
        *   Returns a `PyCapsule` named "scopemux_core.TreeSitterTree". This capsule wraps the raw Tree-sitter tree pointer. The capsule has a destructor set to `ts_tree_free` to manage the C-side memory of the tree.
    *   `tree_to_ir(tree: PyCapsule, parser_ctx: ParserContext)`
        *   Converts a Tree-sitter syntax tree (obtained from `parse_string`) into ScopeMux IR, populating the provided `ParserContext`.
        *   `tree`: The `PyCapsule` returned by `parse_string`.
        *   `parser_ctx`: The `ParserContext` to populate with IR nodes.
    *   `extract_comments(tree: PyCapsule, parser_ctx: ParserContext) -> int`
        *   Extracts comments and docstrings from the Tree-sitter tree and adds them as `ASTNode` objects to the `ParserContext`.
        *   Returns the number of comments/docstrings extracted.
    *   `extract_functions(tree: PyCapsule, parser_ctx: ParserContext) -> int`
        *   Extracts functions and methods from the Tree-sitter tree and adds them as `ASTNode` objects to the `ParserContext`.
        *   Returns the number of functions/methods extracted.
    *   `extract_classes(tree: PyCapsule, parser_ctx: ParserContext) -> int`
        *   Extracts classes, structs, enums, interfaces, etc., from the Tree-sitter tree and adds them as `ASTNode` objects to the `ParserContext`.
        *   Returns the number of class-like structures extracted.
    *   `get_last_error() -> str | None`
        *   Returns the last error message from the Tree-sitter parser operations as a string, or `None` if no error occurred.
