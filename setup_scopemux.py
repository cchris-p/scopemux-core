from setuptools import setup, Extension
import os
import subprocess

# Get Python include path
python_include = "/usr/include/python3.10"  # We know this works from our test

# Define the extension module
scopemux_module = Extension(
    'scopemux_core',
    sources=[
        'bindings/src/bindings/module.c',
        'bindings/src/bindings/parser_bindings.c',
        # Add other source files as needed
    ],
    include_dirs=[
        python_include,
        'bindings/include',
        'bindings/include/scopemux',
    ],
    define_macros=[('PY_SSIZE_T_CLEAN', None)],
    extra_compile_args=['-Wall', '-Wextra'],
)

# Setup the extension
setup(
    name='scopemux',
    version='0.1.0',
    description='ScopeMux Python Bindings',
    ext_modules=[scopemux_module],
)
