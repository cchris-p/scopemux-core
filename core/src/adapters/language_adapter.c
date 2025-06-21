#include "scopemux/adapters/language_adapter.h"
#include "../../include/scopemux/parser.h"

static char *c_extract_signature(TSNode node, const char *source_code) {
  // Implementation moved from extract_full_signature()
}

static void c_post_process_match(ASTNode *node, TSQueryMatch *match) {
  // C-specific match processing
}

LanguageAdapter c_adapter = {.language_type = LANG_C,
                             .language_name = "C",
                             .extract_signature = c_extract_signature,
                             .post_process_match = c_post_process_match};
