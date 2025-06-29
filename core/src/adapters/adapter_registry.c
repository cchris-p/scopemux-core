#include "../../core/include/scopemux/adapters/adapter_registry.h"
#include "../../core/include/scopemux/adapters/language_adapter.h"
#include <stdlib.h> // For NULL

#define MAX_ADAPTERS 10
static LanguageAdapter *registered_adapters[MAX_ADAPTERS] = {NULL};

void register_adapter(LanguageAdapter *adapter) {
  if (!adapter || adapter->language_type >= MAX_ADAPTERS)
    return;
  registered_adapters[adapter->language_type] = adapter;
}

LanguageAdapter *get_adapter(Language lang) {
  if (lang >= MAX_ADAPTERS)
    return NULL;
  return registered_adapters[lang];
}
