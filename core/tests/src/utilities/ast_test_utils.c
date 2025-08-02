#include "../../include/ast_test_utils.h"
#include "../../include/json_validation.h"
#include "scopemux/parser.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Escape a string for JSON output by replacing newlines with \n
 * and other special characters as needed
 * Caller is responsible for freeing the returned string
 */
static char *escape_json_string(const char *str) {
  if (!str)
    return NULL;

  size_t len = strlen(str);
  // Allocate extra space for escaped characters (worst case: every char becomes 2)
  char *escaped = malloc(len * 2 + 1);
  if (!escaped)
    return NULL;

  const char *src = str;
  char *dst = escaped;

  while (*src) {
    switch (*src) {
    case '\n':
      *dst++ = '\\';
      *dst++ = 'n';
      break;
    case '\r':
      *dst++ = '\\';
      *dst++ = 'r';
      break;
    case '\t':
      *dst++ = '\\';
      *dst++ = 't';
      break;
    case '"':
      *dst++ = '\\';
      *dst++ = '"';
      break;
    case '\\':
      *dst++ = '\\';
      *dst++ = '\\';
      break;
    default:
      *dst++ = *src;
      break;
    }
    src++;
  }
  *dst = '\0';

  return escaped;
}

// Get test granularity level from environment variable (set by run_c_tests.sh)
TestGranularityLevel get_test_granularity_level(void) {
  const char *env_level = getenv("TEST_GRANULARITY_LEVEL");
  if (!env_level) {
    // Default level when not set by script (should not happen in normal usage)
    return GRANULARITY_SEMANTIC;
  }

  int level = atoi(env_level);
  switch (level) {
  case 1:
    return GRANULARITY_SMOKE;
  case 2:
    return GRANULARITY_STRUCTURAL;
  case 3:
    return GRANULARITY_SEMANTIC;
  case 4:
    return GRANULARITY_DETAILED;
  case 5:
    return GRANULARITY_EXACT;
  default:
    fprintf(stderr, "Warning: Invalid TEST_GRANULARITY_LEVEL '%s', using default (3)\n", env_level);
    return GRANULARITY_SEMANTIC;
  }
}

// Debug utility: Print ASTNode as JSON (minimal, for test debug output only)
static void print_ast_node_json(const ASTNode *node, int level) {
  if (!node) {
    fprintf(stderr, "null");
    return;
  }
  // Indent
  for (int i = 0; i < level; ++i)
    fprintf(stderr, "  ");
  fprintf(stderr, "{\n");

  for (int i = 0; i < level + 1; ++i)
    fprintf(stderr, "  ");
  fprintf(stderr, "\"type\": \"%s\",\n", ast_node_type_to_string(node->type));
  if (node->name) {
    for (int i = 0; i < level + 1; ++i)
      fprintf(stderr, "  ");
    fprintf(stderr, "\"name\": \"%s\",\n", node->name);
  }
  if (node->qualified_name) {
    for (int i = 0; i < level + 1; ++i)
      fprintf(stderr, "  ");
    fprintf(stderr, "\"qualified_name\": \"%s\",\n", node->qualified_name);
  }
  if (node->signature) {
    for (int i = 0; i < level + 1; ++i)
      fprintf(stderr, "  ");
    fprintf(stderr, "\"signature\": \"%s\",\n", node->signature);
  }
  if (node->docstring) {
    for (int i = 0; i < level + 1; ++i)
      fprintf(stderr, "  ");
    char *escaped_docstring = escape_json_string(node->docstring);
    fprintf(stderr, "\"docstring\": \"%s\",\n", escaped_docstring ? escaped_docstring : "");
    if (escaped_docstring)
      free(escaped_docstring);
  }
  if (node->file_path) {
    for (int i = 0; i < level + 1; ++i)
      fprintf(stderr, "  ");
    fprintf(stderr, "\"file_path\": \"%s\",\n", node->file_path);
  }
  // Print children
  for (int i = 0; i < level + 1; ++i)
    fprintf(stderr, "  ");
  fprintf(stderr, "\"children\": [");
  if (node->num_children > 0)
    fprintf(stderr, "\n");
  for (size_t i = 0; i < node->num_children; ++i) {
    print_ast_node_json(node->children[i], level + 2);
    if (i + 1 < node->num_children)
      fprintf(stderr, ",");
    fprintf(stderr, "\n");
  }
  for (int i = 0; i < level + 1; ++i)
    fprintf(stderr, "  ");
  fprintf(stderr, "]\n");
  for (int i = 0; i < level; ++i)
    fprintf(stderr, "  ");
  fprintf(stderr, "}");
}
ASTTestConfig ast_test_config_init(void) {
  ASTTestConfig config = {.source_file = NULL,
                          .json_file = NULL,
                          .category = NULL,
                          .base_filename = NULL,
                          .language = LANG_UNKNOWN,
                          .debug_mode = true,
                          .granularity_level = get_test_granularity_level()};
  return config;
}

bool has_extension(const char *filename, const char *ext) {
  size_t filename_len = strlen(filename);
  size_t ext_len = strlen(ext);

  if (filename_len <= ext_len) {
    return false;
  }

  return strcmp(filename + filename_len - ext_len, ext) == 0;
}

char *read_file_contents(const char *path) {
  fprintf(stderr, "DEBUG: read_file_contents called with path: %s\n", path);

  // Print current working directory
  char cwd[4096];
  if (getcwd(cwd, sizeof(cwd))) {
    fprintf(stderr, "DEBUG: Current working directory: %s\n", cwd);
  } else {
    perror("getcwd");
  }

  // Print directory listing for the file's directory
  const char *last_slash = strrchr(path, '/');
  if (last_slash) {
    char dirpath[4096];
    size_t len = last_slash - path;
    if (len < sizeof(dirpath)) {
      strncpy(dirpath, path, len);
      dirpath[len] = '\0';
      DIR *dir = opendir(dirpath);
      if (dir) {
        fprintf(stderr, "DEBUG: Directory listing for %s:\n", dirpath);
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
          fprintf(stderr, "  %s\n", entry->d_name);
        }
        closedir(dir);
      } else {
        fprintf(stderr, "DEBUG: Failed to open directory: %s\n", dirpath);
      }
    }
  }

  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "DEBUG: fopen failed for path: %s\n", path);
    fprintf(stderr, "DEBUG: errno=%d (%s)\n", errno, strerror(errno));
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  fprintf(stderr, "DEBUG: file size for %s is %ld bytes\n", path, size);

  char *content = malloc(size + 1);
  if (!content) {
    fprintf(stderr, "DEBUG: malloc failed for %ld bytes for file: %s\n", size + 1, path);
    fclose(file);
    return NULL;
  }

  size_t read_bytes = fread(content, 1, size, file);
  if (read_bytes != size) {
    fprintf(stderr, "DEBUG: fread read %zu bytes, expected %ld bytes for file: %s\n", read_bytes,
            size, path);
  }
  content[size] = '\0';
  fclose(file);

  return content;
}

char *get_absolute_path(const char *relative_path) {
  if (!relative_path) {
    return NULL;
  }

  // If already absolute, just duplicate
  if (relative_path[0] == '/') {
    return strdup(relative_path);
  }

  char cwd[1024];
  if (!getcwd(cwd, sizeof(cwd))) {
    return NULL;
  }

  char *abs_path = malloc(strlen(cwd) + strlen(relative_path) + 2);
  if (!abs_path) {
    return NULL;
  }

  sprintf(abs_path, "%s/%s", cwd, relative_path);
  return abs_path;
}

void json_value_free(JsonValue *value) {
  if (!value) {
    return;
  }

  switch (value->type) {
  case JSON_OBJECT:
    for (size_t i = 0; i < value->value.object.size; i++) {
      free(value->value.object.keys[i]);
      json_value_free(value->value.object.values[i]);
    }
    free(value->value.object.keys);
    free(value->value.object.values);
    break;

  case JSON_ARRAY:
    for (size_t i = 0; i < value->value.array.size; i++) {
      json_value_free(value->value.array.items[i]);
    }
    free(value->value.array.items);
    break;

  case JSON_STRING:
    free(value->value.string);
    break;

  default:
    break; // nothing to free for number, boolean, or null
  }

  free(value);
}

bool run_ast_test(const ASTTestConfig *config) {
  bool test_passed = true;
  char *source_content = NULL;
  ASTNode *ast = NULL;
  JsonValue *expected_json = NULL;

  // Validate input params
  if (!config) {
    cr_assert_fail("NULL config passed to run_ast_test");
    return false;
  }

  // Validate source file
  if (!config->source_file) {
    cr_assert_fail("NULL source_file in test config");
    return false;
  }

  // Validate expected JSON file
  if (!config->json_file) {
    cr_assert_fail("NULL json_file in test config");
    return false;
  }

  cr_log_info("===== BEGIN AST TEST =====");
  cr_log_info("Source file: %s", config->source_file);
  cr_log_info("Expected JSON file: %s", config->json_file);
  cr_log_info("Test granularity level: %d", config->granularity_level);

  // 1. Read the source file
  source_content = read_file_contents(config->source_file);
  if (!source_content) {
    cr_assert_fail("Failed to read source file");
    goto cleanup;
  }

  // Debug: Show source content preview
  size_t src_len = strlen(source_content);
  size_t preview_len = src_len < 200 ? src_len : 200;
  cr_log_info("Source content preview (%zu bytes total):\n%.*s%s", src_len, (int)preview_len,
              source_content, src_len > preview_len ? "..." : "");

  // 2. Parse the file and get the AST
  cr_log_info("Initializing parser context");
  ParserContext *ctx = parser_init();
  if (!ctx) {
    cr_assert_fail("Failed to initialize parser context");
    goto cleanup;
  }

  // Set the language based on the file extension
  const char *extension = get_language_extension(config->language);
  if (!extension) {
    cr_assert_fail("Failed to determine file extension for source file");
    parser_free(ctx);
    goto cleanup;
  }

  cr_log_info("Setting parser language to: %d (extension: %s)", config->language, extension);
  // Commenting this out because it causes issues in the tests
  // parser_set_language(ctx, config->language);

  // Extract filename with extension from source_file path
  const char *filename_with_ext = strrchr(config->source_file, '/');
  if (filename_with_ext) {
    filename_with_ext++; // Skip the '/'
  } else {
    filename_with_ext = config->source_file; // No path separator found
  }

  // Parse the source code
  cr_log_info("Parsing source code with filename: %s", filename_with_ext);
  bool parse_success = parser_parse_string(ctx, source_content, strlen(source_content),
                                           filename_with_ext, config->language);

  if (!parse_success) {
    cr_assert_fail("Failed to parse source file");
    parser_free(ctx);
    goto cleanup;
  }

  // 3. Get AST root
  const ASTNode *ast_root = ctx->ast_root;
  if (!ast_root) {
    cr_assert_fail("AST root node is NULL");
    parser_free(ctx);
    goto cleanup;
  }

  cr_log_info("AST root node exists (type: %d, num_children: %zu)", ast_root->type,
              ast_root->num_children);

  // 4. Load and validate expected JSON
  cr_log_info("Reading expected JSON file");
  char *expected_content = read_file_contents(config->json_file);
  if (!expected_content) {
    cr_assert_fail("Failed to read expected JSON file");
    goto cleanup;
  }

  expected_json = parse_json_string(expected_content);
  if (!expected_json) {
    cr_log_error("Failed to parse expected JSON file");
    // Try to print the first 500 characters for context
    size_t preview_len = strlen(expected_content) > 500 ? 500 : strlen(expected_content);
    fprintf(stderr, "DEBUG: JSON file preview (first %zu bytes):\n%.*s\n", preview_len,
            (int)preview_len, expected_content);
    free(expected_content);
    goto cleanup;
  }

  if (expected_json) {
    // Extract the "ast" section from the expected JSON
    JsonValue *ast_section = find_json_field(expected_json, "ast");
    if (!ast_section) {
      cr_log_error("Expected JSON does not contain 'ast' section");
      test_passed = false;
    } else {
      // Compare AST with expected AST section using granularity-aware validation
      test_passed = validate_ast_with_granularity(ast_root, ast_section, config->granularity_level);
      if (!test_passed) {
        fprintf(stderr, "\n========== AST/JSON MISMATCH =========="
                        "\n");
        fprintf(stderr, "ACTUAL AST (as JSON):\n");
        print_ast_node_json(ast_root, 0);
        fprintf(stderr, "\nEXPECTED AST SECTION:\n");
        print_json_value(ast_section, 0);
        fprintf(stderr, "\n=======================================\n");
      }
    }
    json_value_free(expected_json);
  }

  free(expected_content);

cleanup:
  if (source_content)
    free(source_content);
  if (ctx)
    parser_free(ctx);
  return test_passed;
}

TestPaths construct_test_paths(const char *lang, const char *category, const char *filename) {
  TestPaths paths = {0};

  // Copy filename and get base name without extension
  paths.base_filename = strdup(filename);
  if (paths.base_filename) {
    char *dot = strrchr(paths.base_filename, '.');
    if (dot) {
      *dot = '\0';
    }
  }

  // Construct source and JSON file paths using relative paths
  snprintf(paths.source_path, sizeof(paths.source_path), "core/tests/examples/%s/%s/%s", lang,
           category, filename);

  // Use filename for JSON path to preserve extension
  snprintf(paths.json_path, sizeof(paths.json_path), "core/tests/examples/%s/%s/%s.expected.json",
           lang, category, filename);

  return paths;
}

void process_category_files(const char *lang, const char *category,
                            bool (*is_test_file)(const char *filename),
                            void (*test_file)(const char *category, const char *filename)) {
  char dir_path[1024];
  snprintf(dir_path, sizeof(dir_path), "core/tests/examples/%s/%s", lang, category);

  DIR *dir = opendir(dir_path);
  if (!dir) {
    cr_log_error("Failed to open directory: %s", dir_path);
    return;
  }

  // Collect all test files first
  char **test_files = NULL;
  size_t file_count = 0;
  size_t file_capacity = 10;

  test_files = malloc(file_capacity * sizeof(char *));
  if (!test_files) {
    cr_log_error("Failed to allocate memory for test files list");
    closedir(dir);
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG && is_test_file(entry->d_name)) {
      // Expand array if needed
      if (file_count >= file_capacity) {
        file_capacity *= 2;
        char **new_files = realloc(test_files, file_capacity * sizeof(char *));
        if (!new_files) {
          cr_log_error("Failed to reallocate memory for test files list");
          // Clean up existing allocations
          for (size_t i = 0; i < file_count; i++) {
            free(test_files[i]);
          }
          free(test_files);
          closedir(dir);
          return;
        }
        test_files = new_files;
      }

      // Store filename
      test_files[file_count] = strdup(entry->d_name);
      if (!test_files[file_count]) {
        cr_log_error("Failed to duplicate filename: %s", entry->d_name);
        // Clean up existing allocations
        for (size_t i = 0; i < file_count; i++) {
          free(test_files[i]);
        }
        free(test_files);
        closedir(dir);
        return;
      }
      file_count++;
    }
  }
  closedir(dir);

  // Sort the filenames to ensure consistent order
  for (size_t i = 0; i < file_count - 1; i++) {
    for (size_t j = i + 1; j < file_count; j++) {
      if (strcmp(test_files[i], test_files[j]) > 0) {
        char *temp = test_files[i];
        test_files[i] = test_files[j];
        test_files[j] = temp;
      }
    }
  }

  // Process files in sorted order
  for (size_t i = 0; i < file_count; i++) {
    test_file(category, test_files[i]);
    free(test_files[i]);
  }

  free(test_files);
}

const char *get_language_name(Language lang) {
  switch (lang) {
  case LANG_C:
    return "C";
  case LANG_CPP:
    return "C++";
  case LANG_JAVASCRIPT:
    return "JavaScript";
  case LANG_TYPESCRIPT:
    return "TypeScript";
  case LANG_PYTHON:
    return "Python";
  default:
    return "Unknown";
  }
}

const char *get_language_extension(Language lang) {
  switch (lang) {
  case LANG_C:
    return ".c";
  case LANG_CPP:
    return ".cpp";
  case LANG_JAVASCRIPT:
    return ".js";
  case LANG_TYPESCRIPT:
    return ".ts";
  case LANG_PYTHON:
    return ".py";
  default:
    return "";
  }
}