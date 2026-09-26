"""Public Python surface for the ScopeMux native core.

This package is the supported import path for downstream projects. It re-exports
the bounded public API of the compiled ``scopemux_core`` extension; the extension
module itself is an implementation detail and should not be imported directly by
consumers.

Example:
    >>> import scopemux
    >>> parser = scopemux.ParserContext()
    >>> parser.parse_string("def add(a, b):\\n    return a + b\\n", filename="add.py")
    True
    >>> parser.get_ast_root().get_type()
    'function_definition'
"""

from scopemux_core import (
    ASTNode,
    COMPRESSION_HEAVY,
    COMPRESSION_LIGHT,
    COMPRESSION_MEDIUM,
    COMPRESSION_NONE,
    COMPRESSION_SIGNATURE_ONLY,
    CSTNode,
    DEFAULT_TOKEN_BUDGET,
    ContextEngine,
    InfoBlock,
    LANG_C,
    LANG_CPP,
    LANG_JAVASCRIPT,
    LANG_PYTHON,
    LANG_TYPESCRIPT,
    LANG_UNKNOWN,
    NODE_CLASS,
    NODE_COMMENT,
    NODE_DOCSTRING,
    NODE_ENUM,
    NODE_FUNCTION,
    NODE_INTERFACE,
    NODE_METHOD,
    NODE_MODULE,
    NODE_NAMESPACE,
    NODE_STRUCT,
    NODE_UNKNOWN,
    ParserContext,
    TEST_PROCESSOR_VERSION,
    detect_language,
    parse_c_file_to_cst,
)
from scopemux_core import __version__

__all__ = [
    "ASTNode",
    "COMPRESSION_HEAVY",
    "COMPRESSION_LIGHT",
    "COMPRESSION_MEDIUM",
    "COMPRESSION_NONE",
    "COMPRESSION_SIGNATURE_ONLY",
    "CSTNode",
    "ContextEngine",
    "DEFAULT_TOKEN_BUDGET",
    "InfoBlock",
    "LANG_C",
    "LANG_CPP",
    "LANG_JAVASCRIPT",
    "LANG_PYTHON",
    "LANG_TYPESCRIPT",
    "LANG_UNKNOWN",
    "NODE_CLASS",
    "NODE_COMMENT",
    "NODE_DOCSTRING",
    "NODE_ENUM",
    "NODE_FUNCTION",
    "NODE_INTERFACE",
    "NODE_METHOD",
    "NODE_MODULE",
    "NODE_NAMESPACE",
    "NODE_STRUCT",
    "NODE_UNKNOWN",
    "ParserContext",
    "TEST_PROCESSOR_VERSION",
    "__version__",
    "detect_language",
    "parse_c_file_to_cst",
]
