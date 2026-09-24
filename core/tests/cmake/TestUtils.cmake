# TestUtils.cmake
#
# Shared helpers for the ScopeMux test targets.
#
# The test configuration currently lives inline in core/tests/CMakeLists.txt
# (notably the add_scopemux_test function). This file is the include point for
# reusable test helpers: add shared test utilities here rather than growing the
# top-level test CMakeLists.
#
# This file was previously untracked because the repository .gitignore matched
# *.cmake; the ignore now carries an explicit exception for this path.

include_guard(GLOBAL)
