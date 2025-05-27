from setuptools import setup, Extension, find_packages
import os
import glob

# We know the system Python include path works based on our test
python_include = "/usr/include/python3.10"
print(f"Using Python include path: {python_include}")

# Find all C source files in the bindings directory
source_files = glob.glob('bindings/src/bindings/*.c')
print(f"Source files: {source_files}")

# Add our parser implementation files
parser_sources = [
    'bindings/src/parser/parser.c',
    'bindings/src/parser/tree_sitter_integration.c',
    'bindings/src/parser/tree_sitter_stubs.c',  # Tree-sitter stubs
    'bindings/src/parser/ir_generator.c',
    'bindings/src/context_engine/compressor.c',
    'bindings/src/context_engine/expander.c',
    'bindings/src/context_engine/token_budgeter.c',
    'bindings/src/common/error_handling.c',
    'bindings/src/common/logging.c',
    'bindings/src/common/memory_management.c'
]

# Add Tree-sitter grammar sources
ts_c_sources = ['vendor/tree-sitter-c/src/parser.c']
ts_python_sources = ['vendor/tree-sitter-python/src/parser.c', 'vendor/tree-sitter-python/src/scanner.c']

# Combine all C sources
source_files = source_files + parser_sources + ts_c_sources + ts_python_sources
# Note: We're excluding C++ sources because they require special handling

# Define the extension module
scopemux_module = Extension(
    'scopemux_core',
    sources=source_files,
    include_dirs=[
        python_include,                   # Python include path
        'bindings/include',               # Main include directory
        'bindings/include/scopemux',      # ScopeMux specific headers
        'vendor/tree-sitter/lib/include', # Tree-sitter core headers
        'vendor/tree-sitter-c/src',       # Tree-sitter C grammar headers
        'vendor/tree-sitter-python/src',  # Tree-sitter Python grammar headers
    ],
    define_macros=[('PY_SSIZE_T_CLEAN', None)],
    extra_compile_args=[
        '-Wall',                          # Enable all warnings
        '-Wextra',                        # Enable extra warnings
        '-I/usr/include/python3.10',      # Explicitly add Python include path
    ],
)

# Setup the extension
setup(
    name='scopemux',
    version='0.1.0',
    description='ScopeMux Python Bindings',
    author='ScopeMux Team',
    packages=find_packages(),
    ext_modules=[scopemux_module],
)
