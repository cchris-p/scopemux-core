from setuptools import setup, Extension
import subprocess

# Get Python include path directly from the virtual environment
def get_python_inc():
    cmd = "python -c 'import sysconfig; print(sysconfig.get_path(\"include\"))'"
    return subprocess.check_output(cmd, shell=True).decode('utf-8').strip()

python_include = get_python_inc()
print(f"Using Python include path: {python_include}")

# Define a simple test extension
test_module = Extension(
    'test_scopemux',
    sources=['test_binding.c'],
    include_dirs=[python_include],
)

# Setup the extension
setup(
    name='test_scopemux',
    version='0.1.0',
    description='Test ScopeMux Python Bindings',
    ext_modules=[test_module],
)
