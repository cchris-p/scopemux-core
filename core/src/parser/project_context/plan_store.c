/**
 * @file plan_store.c
 * @brief Durable plan-node store serialization for ScopeMux (`WI-031`).
 *
 * The durable map store holds the projected, target-state plan nodes (lifecycle,
 * provenance, anchors, confidence). It is deliberately separate from the
 * disposable derived store (parse results, IR, InfoBlocks, graph, search
 * index): a re-index refreshes derived data and re-projects plan nodes, but
 * never regenerates plan state. This file provides JSON serialization plus
 * load/save/merge entry points.
 *
 * The JSON schema is stable and machine-readable:
 *
 * @code
 * {
 *   "version": 1,
 *   "plan_nodes": [
 *     {
 *       "id": "plan:TASK-1:add-parser",
 *       "task_id": "TASK-1",
 *       "slug": "add-parser",
 *       "kind": 0,
 *       "lifecycle": 1,
 *       "confidence": 0.5,
 *       "title": "...",
 *       "desired_shape": "...",
 *       "rationale": "...",
 *       "provenance": "...",
 *       "projected_symbol": "...",
 *       "file_path": "...",
 *       "anchors": ["file:/x.c", "sym:helper"]
 *     }
 *   ]
 * }
 * @endcode
 */

#define _POSIX_C_SOURCE 200809L

#include "scopemux/project_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAN_STORE_JSON_VERSION 1

typedef struct {
  char *data;
  size_t length;
  size_t capacity;
} StrBuf;

static bool sb_reserve(StrBuf *buf, size_t extra) {
  char *next;
  size_t capacity;

  if (!buf) {
    return false;
  }
  if (buf->capacity >= buf->length + extra + 1) {
    return true;
  }

  capacity = buf->capacity == 0 ? 256 : buf->capacity;
  while (capacity < buf->length + extra + 1) {
    capacity *= 2;
  }

  next = realloc(buf->data, capacity);
  if (!next) {
    return false;
  }
  buf->data = next;
  buf->capacity = capacity;
  return true;
}

static bool sb_putc(StrBuf *buf, char ch) {
  if (!sb_reserve(buf, 1)) {
    return false;
  }
  buf->data[buf->length++] = ch;
  buf->data[buf->length] = '\0';
  return true;
}

static bool sb_puts(StrBuf *buf, const char *text) {
  size_t length = text ? strlen(text) : 0;
  if (!sb_reserve(buf, length)) {
    return false;
  }
  if (length > 0) {
    memcpy(buf->data + buf->length, text, length);
    buf->length += length;
  }
  buf->data[buf->length] = '\0';
  return true;
}

static bool sb_put_json_string(StrBuf *buf, const char *value) {
  if (!sb_putc(buf, '"')) {
    return false;
  }
  if (value) {
    for (const unsigned char *p = (const unsigned char *)value; *p != '\0'; p++) {
      char escaped[8];
      switch (*p) {
      case '"':
        if (!sb_puts(buf, "\\\"")) {
          return false;
        }
        break;
      case '\\':
        if (!sb_puts(buf, "\\\\")) {
          return false;
        }
        break;
      case '\n':
        if (!sb_puts(buf, "\\n")) {
          return false;
        }
        break;
      case '\r':
        if (!sb_puts(buf, "\\r")) {
          return false;
        }
        break;
      case '\t':
        if (!sb_puts(buf, "\\t")) {
          return false;
        }
        break;
      default:
        if (*p < 0x20) {
          snprintf(escaped, sizeof(escaped), "\\u%04x", (unsigned)*p);
          if (!sb_puts(buf, escaped)) {
            return false;
          }
        } else if (!sb_putc(buf, (char)*p)) {
          return false;
        }
        break;
      }
    }
  }
  return sb_putc(buf, '"');
}

static bool sb_put_key(StrBuf *buf, const char *key) {
  return sb_put_json_string(buf, key) && sb_putc(buf, ':');
}

static bool sb_member(StrBuf *buf, bool *first, const char *key) {
  if (!first || !key) {
    return false;
  }
  if (!*first && !sb_putc(buf, ',')) {
    return false;
  }
  *first = false;
  return sb_put_key(buf, key);
}

char *project_context_plan_nodes_to_json(const ProjectContext *project) {
  StrBuf buf = {0};
  size_t count;

  if (!project) {
    return NULL;
  }

  if (!sb_puts(&buf, "{\"version\":") || !sb_putc(&buf, '0' + PLAN_STORE_JSON_VERSION) ||
      !sb_puts(&buf, ",\"plan_nodes\":[")) {
    free(buf.data);
    return NULL;
  }

  count = project_context_get_plan_node_count(project);
  for (size_t i = 0; i < count; i++) {
    const ProjectPlanNode *node = project_context_get_plan_node_by_index(project, i);
    bool first = true;
    char number[64];

    if (!node) {
      continue;
    }
    if (i > 0 && !sb_putc(&buf, ',')) {
      free(buf.data);
      return NULL;
    }
    if (!sb_putc(&buf, '{') || !sb_member(&buf, &first, "id") ||
        !sb_put_json_string(&buf, node->id) || !sb_member(&buf, &first, "task_id") ||
        !sb_put_json_string(&buf, node->task_id) || !sb_member(&buf, &first, "slug") ||
        !sb_put_json_string(&buf, node->slug) || !sb_member(&buf, &first, "kind")) {
      free(buf.data);
      return NULL;
    }
    snprintf(number, sizeof(number), "%d", (int)node->kind);
    if (!sb_puts(&buf, number) || !sb_member(&buf, &first, "lifecycle")) {
      free(buf.data);
      return NULL;
    }
    snprintf(number, sizeof(number), "%d", (int)node->lifecycle);
    if (!sb_puts(&buf, number) || !sb_member(&buf, &first, "confidence")) {
      free(buf.data);
      return NULL;
    }
    snprintf(number, sizeof(number), "%.6g", (double)node->confidence);
    if (!sb_puts(&buf, number) || !sb_member(&buf, &first, "title") ||
        !sb_put_json_string(&buf, node->title) || !sb_member(&buf, &first, "desired_shape") ||
        !sb_put_json_string(&buf, node->desired_shape) || !sb_member(&buf, &first, "rationale") ||
        !sb_put_json_string(&buf, node->rationale) || !sb_member(&buf, &first, "provenance") ||
        !sb_put_json_string(&buf, node->provenance) ||
        !sb_member(&buf, &first, "projected_symbol") ||
        !sb_put_json_string(&buf, node->projected_symbol) ||
        !sb_member(&buf, &first, "file_path") || !sb_put_json_string(&buf, node->file_path) ||
        !sb_member(&buf, &first, "anchors") || !sb_putc(&buf, '[')) {
      free(buf.data);
      return NULL;
    }
    for (size_t a = 0; a < node->anchor_count; a++) {
      if (a > 0 && !sb_putc(&buf, ',')) {
        free(buf.data);
        return NULL;
      }
      if (!sb_put_json_string(&buf, node->anchor_ids[a])) {
        free(buf.data);
        return NULL;
      }
    }
    if (!sb_puts(&buf, "]}")) {
      free(buf.data);
      return NULL;
    }
  }

  if (!sb_puts(&buf, "]}")) {
    free(buf.data);
    return NULL;
  }

  if (!buf.data) {
    return strdup("{\"version\":1,\"plan_nodes\":[]}");
  }
  return buf.data;
}

/* ------------------------------------------------------------------------- */
/* Minimal JSON reader (scoped to the schema above)                          */
/* ------------------------------------------------------------------------- */

typedef struct {
  const char *text;
  size_t index;
} JsonReader;

typedef struct {
  char *id;
  char *task_id;
  char *slug;
  char *title;
  char *desired_shape;
  char *rationale;
  char *provenance;
  char *projected_symbol;
  char *file_path;
  int kind;
  int lifecycle;
  double confidence;
  char **anchors;
  size_t anchor_count;
  size_t anchor_capacity;
} ParsedPlanNode;

static void parsed_plan_node_free(ParsedPlanNode *node) {
  if (!node) {
    return;
  }
  free(node->id);
  free(node->task_id);
  free(node->slug);
  free(node->title);
  free(node->desired_shape);
  free(node->rationale);
  free(node->provenance);
  free(node->projected_symbol);
  free(node->file_path);
  for (size_t i = 0; i < node->anchor_count; i++) {
    free(node->anchors[i]);
  }
  free(node->anchors);
  memset(node, 0, sizeof(*node));
}

static void jr_skip_ws(JsonReader *r) {
  while (r->text[r->index] == ' ' || r->text[r->index] == '\t' || r->text[r->index] == '\n' ||
         r->text[r->index] == '\r') {
    r->index++;
  }
}

static bool jr_expect(JsonReader *r, char expected) {
  jr_skip_ws(r);
  if (r->text[r->index] != expected) {
    return false;
  }
  r->index++;
  return true;
}

static bool jr_parse_string(JsonReader *r, char **out) {
  StrBuf buf = {0};

  if (!out) {
    return false;
  }
  *out = NULL;

  jr_skip_ws(r);
  if (r->text[r->index] != '"') {
    return false;
  }
  r->index++;

  while (r->text[r->index] != '\0' && r->text[r->index] != '"') {
    char ch = r->text[r->index++];
    if (ch == '\\') {
      char esc = r->text[r->index++];
      switch (esc) {
      case '"':
      case '\\':
      case '/':
        ch = esc;
        break;
      case 'n':
        ch = '\n';
        break;
      case 'r':
        ch = '\r';
        break;
      case 't':
        ch = '\t';
        break;
      case 'b':
        ch = '\b';
        break;
      case 'f':
        ch = '\f';
        break;
      case 'u': {
        char hex[5] = {0};
        for (int i = 0; i < 4; i++) {
          char h = r->text[r->index++];
          if (h == '\0') {
            free(buf.data);
            return false;
          }
          hex[i] = h;
        }
        ch = (char)strtol(hex, NULL, 16);
        break;
      }
      default:
        free(buf.data);
        return false;
      }
    }
    if (!sb_putc(&buf, ch)) {
      free(buf.data);
      return false;
    }
  }

  if (r->text[r->index] != '"') {
    free(buf.data);
    return false;
  }
  r->index++;

  *out = buf.data ? buf.data : strdup("");
  return *out != NULL;
}

static bool jr_parse_number(JsonReader *r, double *out) {
  char *end = NULL;
  double value;

  jr_skip_ws(r);
  value = strtod(r->text + r->index, &end);
  if (end == r->text + r->index) {
    return false;
  }
  r->index = (size_t)(end - r->text);
  if (out) {
    *out = value;
  }
  return true;
}

static bool jr_skip_value(JsonReader *r) {
  jr_skip_ws(r);
  switch (r->text[r->index]) {
  case '"': {
    char *ignored = NULL;
    bool ok = jr_parse_string(r, &ignored);
    free(ignored);
    return ok;
  }
  case '{':
  case '[':
    return false; /* nested containers are not expected in the schema */
  default: {
    double ignored;
    if (strncmp(r->text + r->index, "true", 4) == 0) {
      r->index += 4;
      return true;
    }
    if (strncmp(r->text + r->index, "false", 5) == 0) {
      r->index += 5;
      return true;
    }
    if (strncmp(r->text + r->index, "null", 4) == 0) {
      r->index += 4;
      return true;
    }
    return jr_parse_number(r, &ignored);
  }
  }
}

static bool parsed_node_add_anchor(ParsedPlanNode *node, char *anchor) {
  char **next;
  size_t capacity;

  if (node->anchor_count == node->anchor_capacity) {
    capacity = node->anchor_capacity == 0 ? 4 : node->anchor_capacity * 2;
    next = realloc(node->anchors, capacity * sizeof(*next));
    if (!next) {
      return false;
    }
    node->anchors = next;
    node->anchor_capacity = capacity;
  }
  node->anchors[node->anchor_count++] = anchor;
  return true;
}

static bool jr_parse_anchors(JsonReader *r, ParsedPlanNode *node) {
  if (!jr_expect(r, '[')) {
    return false;
  }
  jr_skip_ws(r);
  if (r->text[r->index] == ']') {
    r->index++;
    return true;
  }
  for (;;) {
    char *anchor = NULL;
    if (!jr_parse_string(r, &anchor)) {
      return false;
    }
    if (!parsed_node_add_anchor(node, anchor)) {
      free(anchor);
      return false;
    }
    jr_skip_ws(r);
    if (r->text[r->index] == ',') {
      r->index++;
      continue;
    }
    if (r->text[r->index] == ']') {
      r->index++;
      return true;
    }
    return false;
  }
}

static bool jr_parse_plan_node(JsonReader *r, ParsedPlanNode *node) {
  if (!jr_expect(r, '{')) {
    return false;
  }

  for (;;) {
    char *key = NULL;
    bool ok = true;

    jr_skip_ws(r);
    if (r->text[r->index] == '}') {
      r->index++;
      break;
    }
    if (!jr_parse_string(r, &key) || !jr_expect(r, ':')) {
      free(key);
      return false;
    }

    if (strcmp(key, "id") == 0) {
      free(node->id);
      ok = jr_parse_string(r, &node->id);
    } else if (strcmp(key, "task_id") == 0) {
      free(node->task_id);
      ok = jr_parse_string(r, &node->task_id);
    } else if (strcmp(key, "slug") == 0) {
      free(node->slug);
      ok = jr_parse_string(r, &node->slug);
    } else if (strcmp(key, "title") == 0) {
      free(node->title);
      ok = jr_parse_string(r, &node->title);
    } else if (strcmp(key, "desired_shape") == 0) {
      free(node->desired_shape);
      ok = jr_parse_string(r, &node->desired_shape);
    } else if (strcmp(key, "rationale") == 0) {
      free(node->rationale);
      ok = jr_parse_string(r, &node->rationale);
    } else if (strcmp(key, "provenance") == 0) {
      free(node->provenance);
      ok = jr_parse_string(r, &node->provenance);
    } else if (strcmp(key, "projected_symbol") == 0) {
      free(node->projected_symbol);
      ok = jr_parse_string(r, &node->projected_symbol);
    } else if (strcmp(key, "file_path") == 0) {
      free(node->file_path);
      ok = jr_parse_string(r, &node->file_path);
    } else if (strcmp(key, "kind") == 0) {
      double value = 0;
      ok = jr_parse_number(r, &value);
      node->kind = (int)value;
    } else if (strcmp(key, "lifecycle") == 0) {
      double value = 0;
      ok = jr_parse_number(r, &value);
      node->lifecycle = (int)value;
    } else if (strcmp(key, "confidence") == 0) {
      ok = jr_parse_number(r, &node->confidence);
    } else if (strcmp(key, "anchors") == 0) {
      ok = jr_parse_anchors(r, node);
    } else {
      ok = jr_skip_value(r);
    }

    free(key);
    if (!ok) {
      return false;
    }

    jr_skip_ws(r);
    if (r->text[r->index] == ',') {
      r->index++;
      continue;
    }
    if (r->text[r->index] == '}') {
      r->index++;
      break;
    }
    return false;
  }

  return node->task_id != NULL && node->slug != NULL;
}

typedef struct {
  ParsedPlanNode *nodes;
  size_t count;
  size_t capacity;
} ParsedPlanStore;

static void parsed_plan_store_free(ParsedPlanStore *store) {
  if (!store) {
    return;
  }
  for (size_t i = 0; i < store->count; i++) {
    parsed_plan_node_free(&store->nodes[i]);
  }
  free(store->nodes);
  memset(store, 0, sizeof(*store));
}

static bool parsed_plan_store_push(ParsedPlanStore *store, ParsedPlanNode *node) {
  ParsedPlanNode *next;
  size_t capacity;

  if (store->count == store->capacity) {
    capacity = store->capacity == 0 ? 8 : store->capacity * 2;
    next = realloc(store->nodes, capacity * sizeof(*next));
    if (!next) {
      return false;
    }
    store->nodes = next;
    store->capacity = capacity;
  }
  store->nodes[store->count++] = *node;
  memset(node, 0, sizeof(*node));
  return true;
}

static bool parse_plan_store(const char *json, ParsedPlanStore *store) {
  JsonReader r = {json, 0};
  int version = 0;
  bool saw_nodes = false;

  if (!jr_expect(&r, '{')) {
    return false;
  }

  for (;;) {
    char *key = NULL;
    bool ok = true;

    jr_skip_ws(&r);
    if (r.text[r.index] == '}') {
      r.index++;
      break;
    }
    if (!jr_parse_string(&r, &key) || !jr_expect(&r, ':')) {
      free(key);
      return false;
    }

    if (strcmp(key, "version") == 0) {
      double value = 0;
      ok = jr_parse_number(&r, &value);
      version = (int)value;
    } else if (strcmp(key, "plan_nodes") == 0) {
      saw_nodes = true;
      if (!jr_expect(&r, '[')) {
        ok = false;
      } else {
        jr_skip_ws(&r);
        if (r.text[r.index] == ']') {
          r.index++;
        } else {
          for (;;) {
            ParsedPlanNode node = {0};
            if (!jr_parse_plan_node(&r, &node)) {
              parsed_plan_node_free(&node);
              ok = false;
              break;
            }
            if (!parsed_plan_store_push(store, &node)) {
              parsed_plan_node_free(&node);
              ok = false;
              break;
            }
            jr_skip_ws(&r);
            if (r.text[r.index] == ',') {
              r.index++;
              continue;
            }
            if (r.text[r.index] == ']') {
              r.index++;
              break;
            }
            ok = false;
            break;
          }
        }
      }
    } else {
      ok = jr_skip_value(&r);
    }

    free(key);
    if (!ok) {
      return false;
    }

    jr_skip_ws(&r);
    if (r.text[r.index] == ',') {
      r.index++;
      continue;
    }
    if (r.text[r.index] == '}') {
      r.index++;
      break;
    }
    return false;
  }

  if (!saw_nodes || version != PLAN_STORE_JSON_VERSION) {
    return false;
  }

  jr_skip_ws(&r);
  return r.text[r.index] == '\0';
}

static bool apply_parsed_node(ProjectContext *project, const ParsedPlanNode *parsed) {
  char id[512];
  ProjectPlanNode *node;
  int written;

  written = snprintf(id, sizeof(id), "plan:%s:%s", parsed->task_id, parsed->slug);
  if (written < 0 || (size_t)written >= sizeof(id)) {
    return false;
  }

  node = project_context_find_plan_node(project, id);
  if (!node) {
    node = project_context_plan_node_create(project, parsed->task_id, parsed->slug,
                                            (ProjectPlanNodeKind)parsed->kind);
    if (!node) {
      return false;
    }
  }

  if (!project_context_plan_node_set_title(project, node, parsed->title) ||
      !project_context_plan_node_set_desired_shape(project, node, parsed->desired_shape) ||
      !project_context_plan_node_set_rationale(project, node, parsed->rationale) ||
      !project_context_plan_node_set_provenance(project, node, parsed->provenance) ||
      !project_context_plan_node_set_projected_symbol(project, node, parsed->projected_symbol) ||
      !project_context_plan_node_set_file_path(project, node, parsed->file_path) ||
      !project_context_plan_node_set_confidence(project, node, (float)parsed->confidence)) {
    return false;
  }

  /* Replace anchors with the persisted set. */
  for (size_t i = 0; i < node->anchor_count; i++) {
    free(node->anchor_ids[i]);
  }
  node->anchor_count = 0;
  for (size_t i = 0; i < parsed->anchor_count; i++) {
    if (!project_context_plan_node_add_anchor(project, node, parsed->anchors[i])) {
      return false;
    }
  }

  return project_context_plan_node_set_lifecycle(project, node,
                                                 (ProjectInfoBlockLifecycle)parsed->lifecycle);
}

bool project_context_plan_nodes_from_json(ProjectContext *project, const char *json, bool merge) {
  ParsedPlanStore store = {0};
  bool ok = true;

  if (!project || !json) {
    return false;
  }

  if (!parse_plan_store(json, &store)) {
    parsed_plan_store_free(&store);
    return false;
  }

  if (!merge) {
    project_context_clear_plan_nodes(project);
  }

  for (size_t i = 0; i < store.count && ok; i++) {
    ok = apply_parsed_node(project, &store.nodes[i]);
  }

  parsed_plan_store_free(&store);
  return ok;
}

bool project_context_plan_nodes_save(const ProjectContext *project, const char *path) {
  char *json;
  FILE *file;
  size_t length;
  bool ok;

  if (!project || !path) {
    return false;
  }

  json = project_context_plan_nodes_to_json(project);
  if (!json) {
    return false;
  }

  file = fopen(path, "wb");
  if (!file) {
    free(json);
    return false;
  }

  length = strlen(json);
  ok = fwrite(json, 1, length, file) == length;
  if (fclose(file) != 0) {
    ok = false;
  }
  free(json);
  return ok;
}

bool project_context_plan_nodes_load(ProjectContext *project, const char *path, bool merge) {
  FILE *file;
  char *buffer;
  long size;
  size_t read;
  bool ok;

  if (!project || !path) {
    return false;
  }

  file = fopen(path, "rb");
  if (!file) {
    return false;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return false;
  }
  size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return false;
  }

  buffer = malloc((size_t)size + 1);
  if (!buffer) {
    fclose(file);
    return false;
  }
  read = fread(buffer, 1, (size_t)size, file);
  fclose(file);
  if (read != (size_t)size) {
    free(buffer);
    return false;
  }
  buffer[size] = '\0';

  ok = project_context_plan_nodes_from_json(project, buffer, merge);
  free(buffer);
  return ok;
}
