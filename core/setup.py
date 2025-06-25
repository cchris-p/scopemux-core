from setuptools import setup, Extension
import sys
import os

# Version should ideally be managed in one place
__version__ = "0.1.0"

# Base project directory and build directory
project_base = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ts_lib_dir = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "build", "tree-sitter-libs"
)

# Determine Python include directory
# This is a more robust way than relying on distutils.sysconfig for some setups
python_include_dirs = []
try:
    from sysconfig import get_paths

    python_include_dirs.append(get_paths()["include"])
except ImportError:
    # Fallback for older Python versions or unusual setups
    from distutils import sysconfig

    python_include_dirs.append(sysconfig.get_python_inc())


ext_modules = [
    Extension(
        "scopemux_core",  # Output module name: import scopemux_core
        sources=[  # All source files needed for the extension
            # Binding files
            "src/bindings/module.c",
            "src/bindings/parser_bindings.c",
            "src/bindings/context_engine_bindings.c",
            "src/bindings/test_processor_bindings.c",
            # Parser files
            "src/parser/parser.c",
            "src/parser/parser_context.c",
            "src/parser/tree_sitter_integration.c",
            "src/parser/query_manager.c",
            "src/parser/query_processing.c",
            "src/parser/memory_tracking.c",
            "src/parser/ast_node.c",
            "src/parser/cst_node.c",
            "src/bindings/signal_handler.c",
            # Context engine files
            "src/context_engine/context_engine.c",
            "src/context_engine/compressor.c",
            "src/context_engine/expander.c",
            "src/context_engine/token_budgeter.c",
            # Common utility files
            "src/common/error_handling.c",
            "src/common/memory_management.c",
            "src/common/logging.c",
            "src/utils/memory_debug.c",
            # Processor files
            "src/processors/test_processor.c",
            "src/processors/ast_post_processor.c",
            "src/processors/docstring_processor.c",
            # Config files
            "src/config/node_type_mapping_loader.c",
            # Adapter files
            "src/adapters/adapter_registry.c",
            "src/adapters/language_adapter.c",
        ],
        include_dirs=[
            "include",  # Relative to this setup.py file
            # Tree-sitter include directories
            os.path.join(project_base, "vendor", "tree-sitter", "lib", "include"),
            os.path.join(project_base, "vendor", "tree-sitter-c", "src"),
            os.path.join(project_base, "vendor", "tree-sitter-cpp", "src"),
            os.path.join(project_base, "vendor", "tree-sitter-python", "src"),
            os.path.join(project_base, "vendor", "tree-sitter-javascript", "src"),
            os.path.join(
                project_base, "vendor", "tree-sitter-typescript", "typescript", "src"
            ),
        ]
        + python_include_dirs,
        define_macros=[
            ("SCOPEMUX_BUILDING", "1"),
            # Py_LIMITED_API can be tricky with manual C API usage if not all types are stable.
            # Consider removing if it causes issues with PyTypeObject definitions.
            # ("Py_LIMITED_API", "0x03070000"),
            ("PY_SSIZE_T_CLEAN", None),  # Recommended for Python C extensions
        ],
        extra_compile_args=[
            "-std=c11",  # C11 standard
            "-Wall",  # Enable all warnings
            "-Wextra",  # Enable extra warnings
            "-pedantic",  # Enforce strict ISO C compliance
            # Add any other platform-specific or necessary compiler flags here
        ],
        # For pure C extensions, 'language' is not typically needed or is 'c'
        # language="c" # Redundant for standard C extensions
        # Library directories where the static libraries are located
        library_dirs=[
            ts_lib_dir,
        ],
        # Libraries to link against
        libraries=[
            "tree-sitter",
            "tree-sitter-c",
            "tree-sitter-cpp",
            "tree-sitter-python",
            "tree-sitter-javascript",
            "tree-sitter-typescript",
        ],
    ),
]

setup(
    name="scopemux_core",  # Distribution name (e.g., for pip)
    version=__version__,
    author="Scopemux Developer",  # Replace with actual author
    author_email="developer@example.com",  # Replace with actual email
    description="Python bindings for Scopemux core C libraries",
    long_description="This package provides the scopemux_core module, which offers Python bindings to the underlying C implementation of Scopemux functionalities.",
    ext_modules=ext_modules,
    # No special cmdclass needed for standard C extensions
    zip_safe=False,  # Generally False for C extensions
    python_requires=">=3.7",
)
