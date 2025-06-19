#include "../../include/json_validation.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>    /* For cr_log_* functions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>            /* For getcwd() */

// Forward declarations of helper functions
static JsonValue *parse_json_value(const char **json);
static void skip_whitespace(const char **json);
static char *parse_string(const char **json);

JsonValue *load_expected_json(const char *language, const char *category, const char *file_name) {
  char filepath[512];
  char project_root_path[512] = "";
  FILE *f = NULL;
  
  // First try using PROJECT_ROOT_DIR environment variable
  const char *project_root = getenv("PROJECT_ROOT_DIR");
  if (project_root) {
    snprintf(project_root_path, sizeof(project_root_path), "%s/core/tests/examples/%s/%s/%s.expected.json", 
             project_root, language, category, file_name);
    f = fopen(project_root_path, "rb");
    if (f) {
      cr_log_info("Successfully opened expected JSON file using PROJECT_ROOT_DIR: %s", project_root_path);
      goto file_found;
    }
  }
  
  // Get current working directory for logging and path calculation
  char cwd[512];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    cr_log_error("Failed to get current working directory");
    return NULL;
  }
  cr_log_info("Current working directory for JSON: %s", cwd);
  
  // Try multiple possible paths based on where the test might be running from
  const char *possible_paths[] = {
    "../../../core/tests/examples/%s/%s/%s.expected.json",  // From build/core/tests/
    "../../core/tests/examples/%s/%s/%s.expected.json",    // One level up
    "../core/tests/examples/%s/%s/%s.expected.json",       // Two levels up
    "../examples/%s/%s/%s.expected.json",                  // Original path
    "./core/tests/examples/%s/%s/%s.expected.json",        // From project root
    "/home/matrillo/apps/scopemux/core/tests/examples/%s/%s/%s.expected.json"  // Absolute path
  };
  
  for (size_t i = 0; i < sizeof(possible_paths) / sizeof(possible_paths[0]); i++) {
    snprintf(filepath, sizeof(filepath), possible_paths[i], language, category, file_name);
    f = fopen(filepath, "rb");
    if (f) {
      cr_log_info("Successfully opened expected JSON file: %s", filepath);
      goto file_found;
    }
  }
  
  // Try to construct paths by navigating from the build directory to the source directory
  if (strstr(cwd, "/build/")) {
    // If we're in a build subdirectory, try to navigate to source
    char build_path[512];
    strcpy(build_path, cwd);
    
    char *build_pos = strstr(build_path, "/build/");
    if (build_pos) {
      *build_pos = '\0'; // Terminate string at /build to get project root
      
      // Construct path from project root to examples
      snprintf(filepath, sizeof(filepath), "%s/core/tests/examples/%s/%s/%s.expected.json", 
               build_path, language, category, file_name);
      f = fopen(filepath, "rb");
      if (f) {
        cr_log_info("Successfully opened expected JSON file using build directory logic: %s", filepath);
        goto file_found;
      }
    }
  }
  
  // If all paths failed
  cr_log_error("Failed to open expected JSON file: %s/%s/%s.expected.json (from working dir: %s)", 
               language, category, file_name, cwd);
  return NULL;

file_found:
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

/**
 * Get string representation of AST node type
 */
static const char *ast_type_to_string(ASTNodeType type) {
  switch (type) {
    case NODE_UNKNOWN:    return "unknown";
    case NODE_FUNCTION:   return "function";
    case NODE_METHOD:     return "method";
    case NODE_CLASS:      return "class";
    case NODE_STRUCT:     return "struct";
    case NODE_ENUM:       return "enum";
    case NODE_INTERFACE:  return "interface";
    case NODE_NAMESPACE:  return "namespace";
    case NODE_MODULE:     return "module";
    case NODE_COMMENT:    return "comment";
    case NODE_DOCSTRING:  return "docstring";
    default:              return "unknown";
  }
}

/**
 * Find an ASTNode child by name
 */
static ASTNode *find_child_by_name(ASTNode *parent, const char *name) {
  if (!parent || !parent->children || parent->num_children == 0 || !name) {
    return NULL;
  }
  
  for (size_t i = 0; i < parent->num_children; i++) {
    ASTNode *child = parent->children[i];
    if (child && child->name && strcmp(child->name, name) == 0) {
      return child;
    }
  }
  
  return NULL;
}

/**
 * Find a JSON object field by name
 */
static JsonValue *find_json_field(JsonValue *obj, const char *field_name) {
  if (!obj || obj->type != JSON_OBJECT || !field_name) {
    return NULL;
  }
  
  for (size_t i = 0; i < obj->value.object.size; i++) {
    if (strcmp(obj->value.object.keys[i], field_name) == 0) {
      return obj->value.object.values[i];
    }
  }
  
  return NULL;
}

bool validate_ast_against_json(ASTNode *node, JsonValue *expected, const char *node_path) {
  if (!node) {
    cr_log_error("%s: AST node is NULL", node_path);
    return false;
  }
  
  if (!expected) {
    cr_log_error("%s: Expected JSON is NULL", node_path);
    return false;
  }

  if (expected->type != JSON_OBJECT) {
    cr_log_error("%s: Expected JSON is not an object", node_path);
    return false;
  }

  bool valid = true;
  char child_path[512];

  // Check node type
  JsonValue *type_field = find_json_field(expected, "type");
  if (type_field) {
    if (type_field->type != JSON_STRING) {
      cr_log_error("%s: 'type' field is not a string", node_path);
      valid = false;
    } else {
      const char *expected_type = type_field->value.string;
      const char *actual_type = ast_type_to_string(node->type);
      
      if (strcmp(expected_type, actual_type) != 0) {
        cr_log_error("%s: Type mismatch - expected '%s', got '%s'", 
                      node_path, expected_type, actual_type);
        valid = false;
      }
    }
  }

  // Check node name
  JsonValue *name_field = find_json_field(expected, "name");
  if (name_field) {
    if (name_field->type != JSON_STRING) {
      cr_log_error("%s: 'name' field is not a string", node_path);
      valid = false;
    } else {
      const char *expected_name = name_field->value.string;
      
      if (!node->name) {
        cr_log_error("%s: Expected name '%s', but node name is NULL", 
                      node_path, expected_name);
        valid = false;
      } else if (strcmp(expected_name, node->name) != 0) {
        cr_log_error("%s: Name mismatch - expected '%s', got '%s'", 
                      node_path, expected_name, node->name);
        valid = false;
      }
    }
  }
  
  // Check qualified name
  JsonValue *qualified_name_field = find_json_field(expected, "qualified_name");
  if (qualified_name_field) {
    if (qualified_name_field->type != JSON_STRING) {
      cr_log_error("%s: 'qualified_name' field is not a string", node_path);
      valid = false;
    } else {
      const char *expected_qname = qualified_name_field->value.string;
      
      if (!node->qualified_name) {
        cr_log_error("%s: Expected qualified_name '%s', but node qualified_name is NULL", 
                      node_path, expected_qname);
        valid = false;
      } else if (strcmp(expected_qname, node->qualified_name) != 0) {
        cr_log_error("%s: Qualified name mismatch - expected '%s', got '%s'", 
                      node_path, expected_qname, node->qualified_name);
        valid = false;
      }
    }
  }
  
  // Check source range
  JsonValue *range_field = find_json_field(expected, "range");
  if (range_field && range_field->type == JSON_OBJECT) {
    JsonValue *start_line = find_json_field(range_field, "start_line");
    if (start_line && start_line->type == JSON_NUMBER) {
      if ((int)start_line->value.number != (int)node->range.start.line) {
        cr_log_error("%s: Start line mismatch - expected %d, got %d",
                      node_path, (int)start_line->value.number, node->range.start.line);
        valid = false;
      }
    }
    
    JsonValue *end_line = find_json_field(range_field, "end_line");
    if (end_line && end_line->type == JSON_NUMBER) {
      if ((int)end_line->value.number != (int)node->range.end.line) {
        cr_log_error("%s: End line mismatch - expected %d, got %d",
                      node_path, (int)end_line->value.number, node->range.end.line);
        valid = false;
      }
    }
  }
  
  // Check signature (for functions and methods)
  JsonValue *signature_field = find_json_field(expected, "signature");
  if (signature_field) {
    if (signature_field->type != JSON_STRING) {
      cr_log_error("%s: 'signature' field is not a string", node_path);
      valid = false;
    } else {
      const char *expected_sig = signature_field->value.string;
      
      if (!node->signature) {
        cr_log_error("%s: Expected signature '%s', but node signature is NULL", 
                      node_path, expected_sig);
        valid = false;
      } else if (strcmp(expected_sig, node->signature) != 0) {
        cr_log_error("%s: Signature mismatch - expected '%s', got '%s'", 
                      node_path, expected_sig, node->signature);
        valid = false;
      }
    }
  }
  
  // Check docstring
  JsonValue *docstring_field = find_json_field(expected, "docstring");
  if (docstring_field) {
    if (docstring_field->type != JSON_STRING) {
      cr_log_error("%s: 'docstring' field is not a string", node_path);
      valid = false;
    } else {
      const char *expected_doc = docstring_field->value.string;
      
      if (!node->docstring) {
        cr_log_error("%s: Expected docstring, but node docstring is NULL", node_path);
        valid = false;
      } else if (strcmp(expected_doc, node->docstring) != 0) {
        cr_log_error("%s: Docstring mismatch", node_path);
        // Don't print full docstrings as they could be very long
        valid = false;
      }
    }
  }
  
  // Check children
  JsonValue *children_field = find_json_field(expected, "children");
  if (children_field) {
    if (children_field->type != JSON_ARRAY) {
      cr_log_error("%s: 'children' field is not an array", node_path);
      valid = false;
    } else {
      // First check if the number of children matches
      size_t expected_children = children_field->value.array.size;
      size_t actual_children = node->num_children;
      
      if (expected_children != actual_children) {
        cr_log_error("%s: Children count mismatch - expected %zu, got %zu", 
                     node_path, expected_children, actual_children);
        valid = false;
      }
      
      // Two strategies for child validation:
      // 1. Positional matching - each child is validated against corresponding expected child by position
      // 2. Name-based matching - each expected child is matched to an actual child by name
      
      // Check if we have a "match_by" field to determine matching strategy
      JsonValue *match_by = find_json_field(expected, "match_children_by");
      bool match_by_name = match_by && match_by->type == JSON_STRING && 
                           strcmp(match_by->value.string, "name") == 0;
      
      if (match_by_name) {
        // Name-based matching
        for (size_t i = 0; i < expected_children; i++) {
          JsonValue *expected_child = children_field->value.array.items[i];
          if (expected_child->type != JSON_OBJECT) {
            cr_log_error("%s: Expected child at index %zu is not an object", node_path, i);
            valid = false;
            continue;
          }
          
          JsonValue *child_name = find_json_field(expected_child, "name");
          if (!child_name || child_name->type != JSON_STRING) {
            cr_log_error("%s: Expected child at index %zu has no valid name", node_path, i);
            valid = false;
            continue;
          }
          
          ASTNode *matching_child = find_child_by_name(node, child_name->value.string);
          if (!matching_child) {
            cr_log_error("%s: No child with name '%s' found", 
                         node_path, child_name->value.string);
            valid = false;
            continue;
          }
          
          snprintf(child_path, sizeof(child_path), "%s.%s", node_path, child_name->value.string);
          if (!validate_ast_against_json(matching_child, expected_child, child_path)) {
            valid = false;
          }
        }
      } else {
        // Positional matching (default)
        size_t max_to_check = (expected_children < actual_children) ? 
                               expected_children : actual_children;
        
        for (size_t i = 0; i < max_to_check; i++) {
          JsonValue *expected_child = children_field->value.array.items[i];
          ASTNode *actual_child = node->children[i];
          
          // Check if the expected child has a name to include in the path
          const char *child_name = "unknown";
          JsonValue *name_field = find_json_field(expected_child, "name");
          if (name_field && name_field->type == JSON_STRING) {
            child_name = name_field->value.string;
          } else if (actual_child->name) {
            child_name = actual_child->name;
          }
          
          snprintf(child_path, sizeof(child_path), "%s.%s[%zu]", node_path, child_name, i);
          if (!validate_ast_against_json(actual_child, expected_child, child_path)) {
            valid = false;
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

// Forward declarations for parsing recursive structures
static JsonValue *parse_json_object(const char **json);
static JsonValue *parse_json_array(const char **json);
static double parse_json_number(const char **json);

// Parse a JSON value of any type
static JsonValue *parse_json_value(const char **json) {
  skip_whitespace(json);

  if (!**json) { // End of string
    return NULL;
  }

  JsonValue *value = (JsonValue *)malloc(sizeof(JsonValue));
  if (!value) {
    cr_log_error("Failed to allocate memory for JSON value");
    return NULL;
  }

  char c = **json;
  switch (c) {
    case '{':
      free(value); // Will be allocated in parse_json_object
      return parse_json_object(json);
      
    case '[':
      free(value); // Will be allocated in parse_json_array
      return parse_json_array(json);
      
    case '"':
      value->type = JSON_STRING;
      value->value.string = parse_string(json);
      if (!value->value.string) {
        free(value);
        return NULL;
      }
      return value;
      
    case 't':
      if (strncmp(*json, "true", 4) == 0) {
        value->type = JSON_BOOLEAN;
        value->value.boolean = true;
        *json += 4;
        return value;
      }
      break;
      
    case 'f':
      if (strncmp(*json, "false", 5) == 0) {
        value->type = JSON_BOOLEAN;
        value->value.boolean = false;
        *json += 5;
        return value;
      }
      break;
      
    case 'n':
      if (strncmp(*json, "null", 4) == 0) {
        value->type = JSON_NULL;
        *json += 4;
        return value;
      }
      break;
      
    default:
      if ((c >= '0' && c <= '9') || c == '-') {
        value->type = JSON_NUMBER;
        value->value.number = parse_json_number(json);
        return value;
      }
      break;
  }

  // Unexpected character
  cr_log_error("Unexpected character in JSON: %c", c);
  free(value);
  return NULL;
}

// Parse a JSON object: {"key": value, ...}
static JsonValue *parse_json_object(const char **json) {
  if (**json != '{') {
    return NULL;
  }
  
  JsonValue *object = (JsonValue *)malloc(sizeof(JsonValue));
  if (!object) {
    cr_log_error("Failed to allocate memory for JSON object");
    return NULL;
  }
  
  object->type = JSON_OBJECT;
  object->value.object.size = 0;
  object->value.object.keys = NULL;
  object->value.object.values = NULL;
  
  (*json)++; // Skip '{'
  skip_whitespace(json);
  
  // Handle empty object
  if (**json == '}') {
    (*json)++;
    return object;
  }
  
  // Temporary arrays to store keys and values
  size_t capacity = 8; // Start with space for 8 key-value pairs
  char **keys = (char **)malloc(sizeof(char *) * capacity);
  JsonValue **values = (JsonValue **)malloc(sizeof(JsonValue *) * capacity);
  
  if (!keys || !values) {
    cr_log_error("Failed to allocate memory for JSON object contents");
    free(keys);
    free(values);
    free(object);
    return NULL;
  }
  
  size_t index = 0;
  
  do {
    skip_whitespace(json);
    
    // Need a string key
    if (**json != '"') {
      cr_log_error("Expected string key in JSON object");
      goto cleanup_error;
    }
    
    // Parse the key
    char *key = parse_string(json);
    if (!key) {
      goto cleanup_error;
    }
    
    skip_whitespace(json);
    
    // Need a colon after the key
    if (**json != ':') {
      cr_log_error("Expected ':' after key in JSON object");
      free(key);
      goto cleanup_error;
    }
    
    (*json)++; // Skip ':'
    skip_whitespace(json);
    
    // Parse the value
    JsonValue *value = parse_json_value(json);
    if (!value) {
      free(key);
      goto cleanup_error;
    }
    
    // Store the key-value pair
    if (index >= capacity) {
      // Need more space, double the capacity
      capacity *= 2;
      char **new_keys = (char **)realloc(keys, sizeof(char *) * capacity);
      JsonValue **new_values = (JsonValue **)realloc(values, sizeof(JsonValue *) * capacity);
      
      if (!new_keys || !new_values) {
        cr_log_error("Failed to reallocate memory for JSON object");
        free(key);
        free_json_value(value);
        goto cleanup_error;
      }
      
      keys = new_keys;
      values = new_values;
    }
    
    keys[index] = key;
    values[index] = value;
    index++;
    
    skip_whitespace(json);
    
    // Need a comma or closing brace
  } while (**json == ',' && ((*json)++, skip_whitespace(json), true));
  
  if (**json != '}') {
    cr_log_error("Expected '}' or ',' in JSON object");
    goto cleanup_error;
  }
  
  (*json)++; // Skip '}'
  
  // Store the final results in the object
  object->value.object.size = index;
  object->value.object.keys = keys;
  object->value.object.values = values;
  
  return object;
  
cleanup_error:
  // Free all allocated keys and values
  for (size_t i = 0; i < index; i++) {
    free(keys[i]);
    free_json_value(values[i]);
  }
  free(keys);
  free(values);
  free(object);
  return NULL;
}

// Parse a JSON array: [value, ...]
static JsonValue *parse_json_array(const char **json) {
  if (**json != '[') {
    return NULL;
  }
  
  JsonValue *array = (JsonValue *)malloc(sizeof(JsonValue));
  if (!array) {
    cr_log_error("Failed to allocate memory for JSON array");
    return NULL;
  }
  
  array->type = JSON_ARRAY;
  array->value.array.size = 0;
  array->value.array.items = NULL;
  
  (*json)++; // Skip '['
  skip_whitespace(json);
  
  // Handle empty array
  if (**json == ']') {
    (*json)++;
    return array;
  }
  
  // Temporary array to store values
  size_t capacity = 8; // Start with space for 8 values
  JsonValue **items = (JsonValue **)malloc(sizeof(JsonValue *) * capacity);
  
  if (!items) {
    cr_log_error("Failed to allocate memory for JSON array items");
    free(array);
    return NULL;
  }
  
  size_t index = 0;
  
  do {
    skip_whitespace(json);
    
    // Parse the value
    JsonValue *value = parse_json_value(json);
    if (!value) {
      goto cleanup_array_error;
    }
    
    // Store the value
    if (index >= capacity) {
      // Need more space, double the capacity
      capacity *= 2;
      JsonValue **new_items = (JsonValue **)realloc(items, sizeof(JsonValue *) * capacity);
      
      if (!new_items) {
        cr_log_error("Failed to reallocate memory for JSON array");
        free_json_value(value);
        goto cleanup_array_error;
      }
      
      items = new_items;
    }
    
    items[index++] = value;
    
    skip_whitespace(json);
    
    // Need a comma or closing bracket
  } while (**json == ',' && ((*json)++, skip_whitespace(json), true));
  
  if (**json != ']') {
    cr_log_error("Expected ']' or ',' in JSON array");
    goto cleanup_array_error;
  }
  
  (*json)++; // Skip ']'
  
  // Store the final results in the array
  array->value.array.size = index;
  array->value.array.items = items;
  
  return array;
  
cleanup_array_error:
  // Free all allocated values
  for (size_t i = 0; i < index; i++) {
    free_json_value(items[i]);
  }
  free(items);
  free(array);
  return NULL;
}

// Parse a JSON number
static double parse_json_number(const char **json) {
  char *end;
  double num = strtod(*json, &end);
  
  // Move the json pointer to the end of the number
  *json = end;
  
  return num;
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
