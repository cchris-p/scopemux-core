#ifndef AST_TEST_UTILS_H
#define AST_TEST_UTILS_H

#include "json_validation.h"
#include "scopemux/parser.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * Configuration for AST testing
 */
typedef struct {
  const char *source_file;                // Path to source file
  const char *json_file;                  // Path to expected JSON file
  const char *category;                   // Test category (subdirectory)
  const char *base_filename;              // Base filename without extension
  Language language;                      // Language enum from parser.h
  bool debug_mode;                        // Enable debug output
  TestGranularityLevel granularity_level; // Validation granularity level
} ASTTestConfig;

/**
 * Path construction for test files
 */
typedef struct {
  char source_path[1024]; // Full path to source file
  char json_path[1024];   // Full path to expected JSON file
  char *base_filename;    // Base filename without extension (caller must free)
} TestPaths;

/**
 * Get test granularity level from environment variable TEST_GRANULARITY_LEVEL
 * (This is set by the hardcoded value in run_c_tests.sh)
 * Returns GRANULARITY_SEMANTIC (3) as default if not set or invalid
 */
TestGranularityLevel get_test_granularity_level(void);

/**
 * Initialize test configuration with default values
 */
ASTTestConfig ast_test_config_init(void);

/**
 * Run AST test with the given configuration
 * Returns true if test passes, false otherwise
 */
bool run_ast_test(const ASTTestConfig *config);

/**
 * Helper function to check if a file has a specific extension
 */
bool has_extension(const char *filename, const char *ext);

/**
 * Helper function to read file contents
 * Caller must free the returned string
 */
char *read_file_contents(const char *path);

/**
 * Helper function to convert relative paths to absolute
 * Returns newly allocated string that caller must free
 */
char *get_absolute_path(const char *relative_path);

/**
 * Construct test paths for a given language, category and filename
 * Returns a TestPaths struct with all necessary paths
 * Caller must free base_filename field
 */
TestPaths construct_test_paths(const char *lang, const char *category, const char *filename);

/**
 * Process all test files in a category directory for a given language
 * @param lang Language identifier (e.g., "c", "cpp", "js", "ts", "python")
 * @param category Category directory name
 * @param is_test_file Function pointer to check if a file should be tested
 * @param test_file Function pointer to run the test on a file
 */
void process_category_files(const char *lang, const char *category,
                            bool (*is_test_file)(const char *filename),
                            void (*test_file)(const char *category, const char *filename));

/**
 * Get the name of a language
 */
const char *get_language_name(Language lang);

/**
 * Get the file extension for a language
 */
const char *get_language_extension(Language lang);

#endif // AST_TEST_UTILS_H