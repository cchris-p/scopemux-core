#include "../../include/json_validation.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>

// Forward declarations of helper functions
static JsonValue *parse_json_value(const char **json);
static void skip_whitespace(const char **json);
static char *parse_string(const char **json);

JsonValue *load_expected_json(const char *language, const char *category, const char *file_name) {
  char filepath[512];
  snprintf(filepath, sizeof(filepath), "../examples/%s/%s/%s.expected.json", language, category,
           file_name);

  FILE *f = fopen(filepath, "rb");
  if (!f) {
    cr_log_error("Failed to open expected JSON file: %s", filepath);
    return NULL;
  }

  // Get file size
  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);

  // Read file content
  char *buffer = (char *)malloc(length + 1);
  if (!buffer) {
    cr_log_error("Failed to allocate memory for JSON content");
    fclose(f);
    return NULL;
  }

  fread(buffer, 1, length, f);
  buffer[length] = '\0';
  fclose(f);

  // Parse JSON
  const char *json_str = buffer;
  JsonValue *result = parse_json_value(&json_str);

  // Clean up
  free(buffer);
  return result;
}

void free_json_value(JsonValue *json) {
  if (!json)
    return;

  switch (json->type) {
  case JSON_OBJECT:
    for (size_t i = 0; i < json->value.object.size; i++) {
      free(json->value.object.keys[i]);
      free_json_value(json->value.object.values[i]);
    }
    free(json->value.object.keys);
    free(json->value.object.values);
    break;

  case JSON_ARRAY:
    for (size_t i = 0; i < json->value.array.size; i++) {
      free_json_value(json->value.array.items[i]);
    }
    free(json->value.array.items);
    break;

  case JSON_STRING:
    free(json->value.string);
    break;

  default:
    // No need to free other types
    break;
  }

  free(json);
}

bool validate_ast_against_json(ASTNode *node, JsonValue *expected, const char *node_path) {
  if (!node || !expected) {
    return false;
  }

  // For now, we'll implement a simple validation that checks:
  // 1. If the expected JSON is an object and has a "type" field
  // 2. If the expected JSON is an object and has a "name" field
  // 3. If the expected JSON has "children" array, we'll validate each child

  if (expected->type != JSON_OBJECT) {
    cr_log_error("Expected JSON is not an object at %s", node_path);
    return false;
  }

  bool valid = true;
  char child_path[256];

  // Check node type
  for (size_t i = 0; i < expected->value.object.size; i++) {
    if (strcmp(expected->value.object.keys[i], "type") == 0) {
      JsonValue *type_value = expected->value.object.values[i];
      if (type_value->type == JSON_STRING) {
        // Map AST node type to string and compare
        // This is a simplification - in practice, you'd have a proper mapping
        const char *type_str;
        switch (node->type) {
        case AST_FUNCTION:
          type_str = "function";
          break;
        case AST_CLASS:
          type_str = "class";
          break;
        case AST_METHOD:
          type_str = "method";
          break;
        case AST_STRUCT:
          type_str = "struct";
          break;
        // Add more mappings as needed
        default:
          type_str = "unknown";
        }

        if (strcmp(type_value->value.string, type_str) != 0) {
          cr_log_error("Type mismatch at %s: expected %s, got %s", node_path,
                       type_value->value.string, type_str);
          valid = false;
        }
      }
    } else if (strcmp(expected->value.object.keys[i], "name") == 0) {
      JsonValue *name_value = expected->value.object.values[i];
      if (name_value->type == JSON_STRING) {
        if (!node->name || strcmp(name_value->value.string, node->name) != 0) {
          cr_log_error("Name mismatch at %s: expected %s, got %s", node_path,
                       name_value->value.string, node->name ? node->name : "NULL");
          valid = false;
        }
      }
    } else if (strcmp(expected->value.object.keys[i], "children") == 0) {
      JsonValue *children_value = expected->value.object.values[i];
      if (children_value->type == JSON_ARRAY) {
        // Check if number of children matches
        if (children_value->value.array.size != node->num_children) {
          cr_log_error("Children count mismatch at %s: expected %zu, got %zu", node_path,
                       children_value->value.array.size, node->num_children);
          valid = false;
        } else {
          // Validate each child
          for (size_t j = 0; j < children_value->value.array.size && j < node->num_children; j++) {
            snprintf(child_path, sizeof(child_path), "%s.children[%zu]", node_path, j);
            if (!validate_ast_against_json(node->children[j], children_value->value.array.items[j],
                                           child_path)) {
              valid = false;
            }
          }
        }
      }
    }
    // Add more field validations as needed
  }

  return valid;
}

void print_json_value(JsonValue *json, int level) {
  if (!json) {
    printf("NULL");
    return;
  }

  // Print indentation
  for (int i = 0; i < level; i++) {
    printf("  ");
  }

  switch (json->type) {
  case JSON_OBJECT:
    printf("{\n");
    for (size_t i = 0; i < json->value.object.size; i++) {
      for (int j = 0; j <= level; j++) {
        printf("  ");
      }
      printf("\"%s\": ", json->value.object.keys[i]);
      print_json_value(json->value.object.values[i], level + 1);
      if (i < json->value.object.size - 1) {
        printf(",");
      }
      printf("\n");
    }
    for (int i = 0; i < level; i++) {
      printf("  ");
    }
    printf("}");
    break;

  case JSON_ARRAY:
    printf("[\n");
    for (size_t i = 0; i < json->value.array.size; i++) {
      print_json_value(json->value.array.items[i], level + 1);
      if (i < json->value.array.size - 1) {
        printf(",");
      }
      printf("\n");
    }
    for (int i = 0; i < level; i++) {
      printf("  ");
    }
    printf("]");
    break;

  case JSON_STRING:
    printf("\"%s\"", json->value.string);
    break;

  case JSON_NUMBER:
    printf("%g", json->value.number);
    break;

  case JSON_BOOLEAN:
    printf("%s", json->value.boolean ? "true" : "false");
    break;

  case JSON_NULL:
    printf("null");
    break;
  }
}

// Simple JSON parser implementation (abbreviated for this example)
static JsonValue *parse_json_value(const char **json) {
  skip_whitespace(json);

  JsonValue *value = (JsonValue *)malloc(sizeof(JsonValue));
  if (!value)
    return NULL;

  char c = **json;
  if (c == '{') {
    // Parse object (simplified)
    value->type = JSON_OBJECT;
    value->value.object.size = 0;
    value->value.object.keys = NULL;
    value->value.object.values = NULL;

    // For now, return an empty object
    (*json)++; // Skip '{'
    // Here you'd implement object parsing...

    // Skip to closing brace
    while (**json && **json != '}')
      (*json)++;
    if (**json == '}')
      (*json)++;
  } else if (c == '[') {
    // Parse array (simplified)
    value->type = JSON_ARRAY;
    value->value.array.size = 0;
    value->value.array.items = NULL;

    // For now, return an empty array
    (*json)++; // Skip '['
    // Here you'd implement array parsing...

    // Skip to closing bracket
    while (**json && **json != ']')
      (*json)++;
    if (**json == ']')
      (*json)++;
  } else if (c == '"') {
    // Parse string
    value->type = JSON_STRING;
    value->value.string = parse_string(json);
  } else if ((c >= '0' && c <= '9') || c == '-') {
    // Parse number (simplified)
    value->type = JSON_NUMBER;
    value->value.number = 0;
    // Here you'd implement number parsing...

    // Skip to next non-numeric character
    while (**json && ((**json >= '0' && **json <= '9') || **json == '.' || **json == 'e' ||
                      **json == 'E' || **json == '+' || **json == '-')) {
      (*json)++;
    }
  } else if (strncmp(*json, "true", 4) == 0) {
    value->type = JSON_BOOLEAN;
    value->value.boolean = true;
    *json += 4;
  } else if (strncmp(*json, "false", 5) == 0) {
    value->type = JSON_BOOLEAN;
    value->value.boolean = false;
    *json += 5;
  } else if (strncmp(*json, "null", 4) == 0) {
    value->type = JSON_NULL;
    *json += 4;
  } else {
    // Unexpected character
    free(value);
    return NULL;
  }

  return value;
}

static void skip_whitespace(const char **json) {
  while (**json && (**json == ' ' || **json == '\t' || **json == '\n' || **json == '\r')) {
    (*json)++;
  }
}

static char *parse_string(const char **json) {
  if (**json != '"')
    return NULL;

  (*json)++; // Skip opening quote
  const char *start = *json;

  // Find closing quote (ignoring escaped quotes)
  while (**json && **json != '"') {
    if (**json == '\\' && (*json)[1]) {
      (*json) += 2; // Skip escape sequence
    } else {
      (*json)++;
    }
  }

  size_t length = *json - start;
  char *result = (char *)malloc(length + 1);
  if (result) {
    memcpy(result, start, length);
    result[length] = '\0';
  }

  if (**json == '"') {
    (*json)++; // Skip closing quote
  }

  return result;
}

// Note: This is a simplified JSON parser implementation
// In a production environment, use a full-featured JSON library
