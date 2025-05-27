#ifndef TREE_SITTER_API_H_
#define TREE_SITTER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct TSLanguage TSLanguage;
typedef struct TSParser TSParser;
typedef struct TSTree TSTree;
typedef struct TSNode TSNode;
typedef struct TSCursor TSCursor;
typedef struct TSQuery TSQuery;
typedef struct TSQueryCursor TSQueryCursor;

typedef enum {
    TSInputEncodingUTF8,
    TSInputEncodingUTF16,
} TSInputEncoding;

typedef struct {
    uint32_t row;
    uint32_t column;
} TSPoint;

typedef struct {
    TSPoint start_point;
    TSPoint end_point;
    uint32_t start_byte;
    uint32_t end_byte;
} TSRange;

typedef struct {
    const void *payload;
    const char *(*read)(void *payload, uint32_t byte_index, TSPoint position, uint32_t *bytes_read);
    TSInputEncoding encoding;
} TSInput;

typedef enum {
    TSLogTypeParse,
    TSLogTypeLex,
} TSLogType;

typedef struct {
    void *payload;
    void (*log)(void *payload, TSLogType type, const char *message);
} TSLogger;

// Parser
TSParser *ts_parser_new(void);
void ts_parser_delete(TSParser *parser);
bool ts_parser_set_language(TSParser *parser, const TSLanguage *language);
const TSLanguage *ts_parser_language(const TSParser *parser);
TSTree *ts_parser_parse_string(TSParser *parser, const TSTree *old_tree, const char *string, uint32_t length);
TSTree *ts_parser_parse(TSParser *parser, const TSTree *old_tree, TSInput input);
void ts_parser_set_logger(TSParser *parser, TSLogger logger);
void ts_parser_print_dot_graphs(TSParser *parser, FILE *file);
void ts_parser_set_timeout_micros(TSParser *parser, uint64_t timeout);
uint64_t ts_parser_timeout_micros(const TSParser *parser);

// Tree
void ts_tree_delete(TSTree *tree);
TSNode ts_tree_root_node(const TSTree *tree);
void ts_tree_edit(TSTree *tree, const TSInputEdit *edit);
TSRange *ts_tree_get_changed_ranges(const TSTree *old_tree, const TSTree *new_tree, uint32_t *length);
const TSLanguage *ts_tree_language(const TSTree *tree);

// Node
TSNode ts_node_child(TSNode node, uint32_t child_index);
TSNode ts_node_named_child(TSNode node, uint32_t child_index);
uint32_t ts_node_child_count(TSNode node);
uint32_t ts_node_named_child_count(TSNode node);
TSNode ts_node_parent(TSNode node);
TSNode ts_node_next_sibling(TSNode node);
TSNode ts_node_prev_sibling(TSNode node);
TSNode ts_node_next_named_sibling(TSNode node);
TSNode ts_node_prev_named_sibling(TSNode node);
TSNode ts_node_first_child_for_byte(TSNode node, uint32_t byte);
TSNode ts_node_first_named_child_for_byte(TSNode node, uint32_t byte);
TSNode ts_node_descendant_for_byte_range(TSNode node, uint32_t start_byte, uint32_t end_byte);
TSNode ts_node_descendant_for_point_range(TSNode node, TSPoint start_point, TSPoint end_point);
TSNode ts_node_named_descendant_for_byte_range(TSNode node, uint32_t start_byte, uint32_t end_byte);
TSNode ts_node_named_descendant_for_point_range(TSNode node, TSPoint start_point, TSPoint end_point);
bool ts_node_eq(TSNode node1, TSNode node2);
bool ts_node_is_null(TSNode node);
bool ts_node_is_named(TSNode node);
bool ts_node_is_missing(TSNode node);
bool ts_node_is_extra(TSNode node);
bool ts_node_has_changes(TSNode node);
bool ts_node_has_error(TSNode node);
const char *ts_node_type(TSNode node);
TSSymbol ts_node_symbol(TSNode node);
uint32_t ts_node_start_byte(TSNode node);
uint32_t ts_node_end_byte(TSNode node);
TSPoint ts_node_start_point(TSNode node);
TSPoint ts_node_end_point(TSNode node);
const char *ts_node_string(TSNode node);

// Cursor
TSCursor *ts_cursor_new(void);
void ts_cursor_delete(TSCursor *cursor);
void ts_cursor_reset(TSCursor *cursor, TSNode node);
bool ts_cursor_goto_first_child(TSCursor *cursor);
bool ts_cursor_goto_next_sibling(TSCursor *cursor);
bool ts_cursor_goto_parent(TSCursor *cursor);
TSNode ts_cursor_current_node(const TSCursor *cursor);

// Query
TSQuery *ts_query_new(const TSLanguage *language, const char *source, uint32_t source_len, uint32_t *error_offset, TSQueryError *error_type);
void ts_query_delete(TSQuery *query);
uint32_t ts_query_pattern_count(const TSQuery *query);
uint32_t ts_query_capture_count(const TSQuery *query);
uint32_t ts_query_string_count(const TSQuery *query);
const char *ts_query_capture_name_for_id(const TSQuery *query, uint32_t id, uint32_t *length);
bool ts_query_disable_capture(TSQuery *query, const char *name, uint32_t length);
bool ts_query_disable_pattern(TSQuery *query, uint32_t pattern_index);
void ts_query_predicates_for_pattern(const TSQuery *query, uint32_t pattern_index, uint32_t *length);
const TSQueryPredicateStep *ts_query_step_is_definite(const TSQuery *query, uint32_t byte_offset);

// Query cursor
TSQueryCursor *ts_query_cursor_new(void);
void ts_query_cursor_delete(TSQueryCursor *cursor);
void ts_query_cursor_exec(TSQueryCursor *cursor, const TSQuery *query, TSNode node);
bool ts_query_cursor_next_match(TSQueryCursor *cursor, TSQueryMatch *match);
void ts_query_cursor_set_byte_range(TSQueryCursor *cursor, uint32_t start_byte, uint32_t end_byte);
void ts_query_cursor_set_point_range(TSQueryCursor *cursor, TSPoint start_point, TSPoint end_point);

// Language
uint32_t ts_language_symbol_count(const TSLanguage *language);
const char *ts_language_symbol_name(const TSLanguage *language, TSSymbol symbol);
TSSymbol ts_language_symbol_for_name(const TSLanguage *language, const char *name, uint32_t length, bool is_named);
uint32_t ts_language_field_count(const TSLanguage *language);
const char *ts_language_field_name_for_id(const TSLanguage *language, TSFieldId field_id);
TSFieldId ts_language_field_id_for_name(const TSLanguage *language, const char *name, uint32_t length);
TSSymbol ts_language_symbol_for_field_id(const TSLanguage *language, TSFieldId field_id, uint32_t symbol_index);
uint32_t ts_language_version(const TSLanguage *language);

#ifdef __cplusplus
}
#endif

#endif  // TREE_SITTER_API_H_
