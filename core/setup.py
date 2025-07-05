from setuptools import setup, Extension
import sys
import os
import shutil

# Version should ideally be managed in one place
__version__ = "0.1.0"

# Base project directory and build directory
project_base = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ts_lib_dir = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "build", "tree-sitter-libs"
)

# Ensure the target directory exists
target_dir = os.path.join(project_base, "build", "core")
os.makedirs(target_dir, exist_ok=True)

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


# Custom build_ext command to move the built extension to our target directory
from setuptools.command.build_ext import build_ext as _build_ext


class build_ext(_build_ext):
    def finalize_options(self):
        _build_ext.finalize_options(self)
        # Force the build directory to be in our target location
        self.build_lib = target_dir

    def run(self):
        _build_ext.run(self)
        print(f"Extension built directly to {target_dir}")

        # Create a symlink with a simpler name for easier importing
        for output in self.get_outputs():
            if output.endswith(".so"):
                base_name = os.path.basename(output)
                simple_name = "scopemux_core.so"
                simple_path = os.path.join(os.path.dirname(output), simple_name)

                # Remove existing symlink if it exists
                if os.path.exists(simple_path) and os.path.islink(simple_path):
                    os.unlink(simple_path)
                elif os.path.exists(simple_path):
                    os.remove(simple_path)

                # Create symlink or copy file
                try:
                    os.symlink(base_name, simple_path)
                    print(f"Created symlink: {simple_path} -> {base_name}")
                except (OSError, AttributeError):
                    # Fallback to copy if symlink is not available or fails
                    shutil.copy2(output, simple_path)
                    print(f"Copied file: {output} -> {simple_path}")


ext_modules = [
    Extension(
        "scopemux_core",  # Output module name: import scopemux_core
        sources=[  # All core, parser, processor, adapter, context engine, and utility sources for the extension
            # === Bindings ===
            "src/bindings/module.c",
            "src/bindings/parser_bindings.c",
            "src/bindings/context_engine_bindings.c",
            "src/bindings/test_processor_bindings.c",
            "src/bindings/signal_handler.c",
            # === Parser Core ===
            "src/parser/parser.c",
            "src/parser/parser_context.c",
            "src/parser/parser_context_utils.c",
            "src/parser/ast_node.c",
            "src/parser/cst_node.c",
            "src/parser/memory_tracking.c",
            "src/parser/ast_compliance.c",
            "src/parser/lang/lang_compliance_registry.c",
            "src/parser/lang/lang_compliance.c",
            "src/parser/lang/c_compliance.c",
            "src/parser/lang/c_ast_compliance.c",
            "src/parser/lang/python_ast_compliance.c",
            "src/parser/lang/javascript_ast_compliance.c",
            "src/parser/lang/typescript_ast_compliance.c",
            "src/parser/ts_ast_builder.c",
            "src/parser/ts_cst_builder.c",
            "src/parser/ts_init.c",
            "src/parser/ts_diagnostic.c",
            "src/parser/ts_query_processor.c",
            "src/parser/query_manager.c",
            "src/parser/query_processing.c",
            "src/parser/reference_resolver.c",
            "src/parser/reference_resolvers/language_resolvers.c",
            "src/parser/reference_resolvers/c_resolver.c",
            "src/parser/reference_resolvers/cpp_resolver.c",
            "src/parser/reference_resolvers/javascript_resolver.c",
            "src/parser/reference_resolvers/python_resolver.c",
            "src/parser/reference_resolvers/typescript_resolver.c",
            "src/parser/reference_resolvers/reference_resolver_core.c",
            "src/parser/reference_resolvers/reference_resolver_facade.c",
            "src/parser/reference_resolvers/resolver_core.c",
            "src/parser/reference_resolvers/resolver_implementation.c",
            "src/parser/reference_resolvers/resolver_registration.c",
            "src/parser/reference_resolvers/resolver_resolution.c",
            "src/parser/project_context.c",
            "src/parser/project_context/dependency_management.c",
            "src/parser/project_context/file_management.c",
            "src/parser/project_context/project_context_facade.c",
            "src/parser/project_context/project_utils.c",
            "src/parser/project_context/project_symbol_extraction.c",
            "src/parser/project_context/symbol_management.c",
            "src/parser/project_context/symbol_collection.c",
            "src/parser/symbol_table.c",
            "src/parser/symbol_table/symbol_core.c",
            "src/parser/symbol_table/symbol_entry.c",
            "src/parser/symbol_table/symbol_lookup.c",
            "src/parser/symbol_table/symbol_table_registration.c",
            "src/parser/tree_sitter_integration.c",
            # === Processors ===
            "src/processors/ast_post_processor.c",
            "src/processors/docstring_processor.c",
            "src/processors/test_processor.c",
            # === Adapters ===
            "src/adapters/adapter_registry.c",
            "src/adapters/language_adapter.c",
            # === Context Engine ===
            "src/context_engine/context_engine.c",
            "src/context_engine/compressor.c",
            "src/context_engine/expander.c",
            "src/context_engine/token_budgeter.c",
            # === Common / Utilities ===
            "src/common/error_handling.c",
            "src/common/logging.c",
            "src/common/memory_management.c",
            "src/utils/memory_debug.c",
            "src/utils/ts_resource_manager.c",
            # === Config ===
            "src/config/node_type_mapping_loader.c",
            "src/config/config_loader.c",
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
            "-fsanitize=address",  # AddressSanitizer
            "-fno-omit-frame-pointer",  # Needed for ASan stack traces
            "-g",  # Debug info for ASan
            # Add any other platform-specific or necessary compiler flags here
        ],
        extra_link_args=[
            "-fsanitize=address",
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
    cmdclass={"build_ext": build_ext},  # Use our custom build_ext
    zip_safe=False,  # Generally False for C extensions
    python_requires=">=3.7",
)
