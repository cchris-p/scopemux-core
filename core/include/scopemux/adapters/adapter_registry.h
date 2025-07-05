#ifndef SCOPEMUX_ADAPTER_REGISTRY_H
#define SCOPEMUX_ADAPTER_REGISTRY_H

#include "language_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

void register_adapter(LanguageAdapter *adapter);
LanguageAdapter *get_adapter(Language lang);

#ifdef __cplusplus
}
#endif

#endif // SCOPEMUX_ADAPTER_REGISTRY_H
