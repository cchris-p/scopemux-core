#!/bin/bash
# Find Python.h header location

echo "Python executable path:"
which python

echo "Python version:"
python --version

echo "Python include directories:"
python -c "import sysconfig; print(sysconfig.get_path('include'))"

echo "Python config variables:"
python -c "import sysconfig; print(sysconfig.get_config_vars('INCLUDEPY'))"

echo "Full system Python include path:"
python -c "import distutils.sysconfig; print(distutils.sysconfig.get_python_inc())"

echo "Additional include path check:"
find ~/.pyenv -name Python.h | head -5
