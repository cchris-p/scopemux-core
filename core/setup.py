from setuptools import setup, Extension
import sys

# Version should ideally be managed in one place
__version__ = "0.1.0"

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
        sources=[
            # Binding files
            "src/bindings/module.c",
            "src/bindings/parser_bindings.c",
            "src/bindings/context_engine_bindings.c",
            "src/bindings/tree_sitter_bindings.c",
            # Parser core files
            "src/parser/parser.c",
            "src/parser/ir_generator.c",
            "src/parser/tree_sitter_integration.c",
            # Context engine files
            "src/context_engine/compressor.c",
            "src/context_engine/expander.c",
            "src/context_engine/token_budgeter.c",
            # Common utility files
            "src/common/error_handling.c",
            "src/common/memory_management.c",
            "src/common/logging.c",
        ],
        include_dirs=[
            "include",  # Relative to this setup.py file
            "include/scopemux",
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
