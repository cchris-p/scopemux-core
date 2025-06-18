/**
 * @file c_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for C language
 *
 * This file will contain tests that iterate through each subdirectory of the
 * /home/matrillo/apps/scopemux/core/tests/examples/c directory, load C source files,
 * extract their ASTs, and validate them against corresponding .expected.json files.
 *
 * Planned subdirectory coverage:
 * - core_tests/examples/c/basic_syntax/
 * - core_tests/examples/c/core_constructs/
 * - core_tests/examples/c/control_flow/
 * - core_tests/examples/c/preprocessor/
 * - Any other directories added to examples/c/
 *
 * Each test will:
 * 1. Read a C source file from examples
 * 2. Parse it into an AST
 * 3. Load the corresponding .expected.json file
 * 4. Compare the AST against the expected JSON output
 * 5. Report any discrepancies
 *
 * This approach provides both regression testing and documentation of
 * the expected parser output for different C language constructs.
 */

// TODO: Implement actual tests once JSON validation is fully implemented
