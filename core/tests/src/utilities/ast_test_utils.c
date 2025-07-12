#include "../../include/ast_test_utils.h"
#include "scopemux/parser.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ASTTestConfig ast_test_config_init(void) {
  ASTTestConfig config = {.source_file = NULL,
                          .json_file = NULL,
                          .category = NULL,
                          .base_filename = NULL,
                          .language = LANG_UNKNOWN,
                          .debug_mode = true};
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
  if (!config || !config->source_file) {
    if (config->debug_mode) {
      fprintf(stderr, "TESTING: Invalid test configuration\n");
    }
    return false;
  }

  // 1. Read source file
  if (config->debug_mode) {
    fprintf(stderr, "TESTING: Reading source file %s...\n", config->source_file);
  }

  char *source = read_file_contents(config->source_file);
  if (!source) {
    if (config->debug_mode) {
      fprintf(stderr, "TESTING: Failed to read source file %s\n", config->source_file);
    }
    return false;
  }

  if (config->debug_mode) {
    fprintf(stderr, "TESTING: Successfully read source file (length: %zu bytes)\n", strlen(source));
  }

  // 2. Initialize parser and parse source
  if (config->debug_mode) {
    fprintf(stderr, "TESTING: Initializing parser context...\n");
  }

  ParserContext *ctx = parser_init();
  if (!ctx) {
    if (config->debug_mode) {
      fprintf(stderr, "TESTING: Failed to create parser context\n");
    }
    free(source);
    return false;
  }

  parser_set_mode(ctx, PARSE_AST);
  bool parse_success =
      parser_parse_string(ctx, source, strlen(source), config->base_filename, config->language);

  if (!parse_success) {
    if (config->debug_mode) {
      fprintf(stderr, "TESTING: Failed to parse source code\n");
      const char *error = parser_get_last_error(ctx);
      if (error) {
        fprintf(stderr, "TESTING: Parser error: %s\n", error);
      }
    }
    free(source);
    parser_free(ctx);
    return false;
  }

  if (config->debug_mode) {
    fprintf(stderr, "TESTING: Source code parsed successfully\n");
  }

  // 3. Get AST root
  const ASTNode *ast = ctx->ast_root;
  if (!ast) {
    if (config->debug_mode) {
      fprintf(stderr, "TESTING: AST root node is NULL\n");
    }
    free(source);
    parser_free(ctx);
    return false;
  }

  if (config->debug_mode) {
    fprintf(stderr, "TESTING: AST root node exists (type: %d, num_children: %zu)\n", ast->type,
            ast->num_children);
  }

  // 4. Load and validate expected JSON
  if (config->debug_mode && config->json_file) {
    if (config->debug_mode) {
      fprintf(stderr, "[%s Test: %s] Loading expected JSON from %s...\n",
              get_language_name(config->language), config->base_filename, config->json_file);
    }
  }

  bool test_passed = true;
  if (config->json_file) {
    JsonValue *expected_json = NULL;
    char *json_content = read_file_contents(config->json_file);

    if (!json_content) {
      if (config->debug_mode) {
        fprintf(stderr, "DEBUG: Failed to read JSON file: %s\n", config->json_file);
      }
    } else {
      if (config->debug_mode) {
        fprintf(stderr, "DEBUG: Read JSON file (%zu bytes): %s\n", strlen(json_content),
                config->json_file);
      }
      expected_json = parse_json_string(json_content);
      if (!expected_json) {
        if (config->debug_mode) {
          fprintf(stderr, "DEBUG: parse_json_string returned NULL. Possible reasons: "
                          "out-of-memory, stack overflow, or malformed JSON.\n");
          // Try to print the first 500 characters for context
          size_t preview_len = strlen(json_content) > 500 ? 500 : strlen(json_content);
          fprintf(stderr, "DEBUG: JSON file preview (first %zu bytes):\n%.*s\n", preview_len,
                  (int)preview_len, json_content);
        }
      }
      free(json_content);
    }

    if (!expected_json) {
      if (config->debug_mode) {
        fprintf(stderr, "TESTING: Failed to load or parse expected JSON\n");
      }
      test_passed = false;
    } else {
      // Compare AST with expected JSON
      test_passed = validate_ast_against_json(ast, expected_json);
      json_value_free(expected_json);
    }
  }

  // Cleanup
  free(source);
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

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG && is_test_file(entry->d_name)) {
      test_file(category, entry->d_name);
    }
  }

  closedir(dir);
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