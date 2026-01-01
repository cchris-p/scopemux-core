#!/bin/bash

cd /home/matrillo/apps/scopemux-core/build-c

# Set the same environment variables as the test runner
export SCOPEMUX_TEST_FILE="core/tests/core/tests/examples/c/basic_syntax/hello_world.c"
export SCOPEMUX_EXPECTED_JSON="core/tests/core/tests/examples/c/basic_syntax/hello_world.c.expected.json"
export TEST_GRANULARITY_LEVEL=3

echo "=== Running test directly ==="
./core/tests/c_example_ast_tests
echo "Direct exit code: $?"

echo ""
echo "=== Running test with output redirection (like test runner) ==="
./core/tests/c_example_ast_tests >/tmp/test_output.log 2>&1
test_result=$?
echo "Redirected exit code: $test_result"

echo ""
echo "=== Output from redirected test ==="
tail -10 /tmp/test_output.log
