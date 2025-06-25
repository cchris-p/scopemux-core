#pragma once

#include "language_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

void register_adapter(LanguageAdapter *adapter);
LanguageAdapter *get_adapter(LanguageType lang);

#ifdef __cplusplus
}
#endif
