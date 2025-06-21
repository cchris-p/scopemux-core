#pragma once

#include <stddef.h> // For size_t

typedef struct ParserContext ParserContext;
typedef struct ASTNode ASTNode;

// Minimal function declarations needed for tests
ParserContext *parser_init(void);
// NOTE: The parser_parse_string declaration must always match the canonical signature in parser.h.
// LanguageType is defined in the main parser headers. Do not redefine it here.
bool parser_parse_string(ParserContext *ctx, const char *const content, size_t content_length,
                         const char *filename, LanguageType language); // Must match parser.h
const char *parser_get_last_error(const ParserContext *ctx);
void parser_free(ParserContext *ctx);
