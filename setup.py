from setuptools import setup, Extension, find_packages
import glob

# We know the system Python include path works based on our test
python_include = "/usr/include/python3.10"
print(f"Using Python include path: {python_include}")

# Find all source files in the bindings directory
source_files = glob.glob('bindings/src/bindings/*.c')
print(f"Source files: {source_files}")

# Define the extension module
scopemux_module = Extension(
    'scopemux_core',
    sources=source_files,
    include_dirs=[
        python_include,                   # Python include path
        'bindings/include',               # Main include directory
        'bindings/include/scopemux',      # ScopeMux specific headers
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
