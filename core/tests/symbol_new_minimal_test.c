#include <stdio.h>
#include "reference_resolvers/reference_resolver_private.h"

int main() {
    printf("[MINIMAL TEST] About to call symbol_new...\n");
    Symbol *sym = symbol_new("minimal_test", SYMBOL_FUNCTION);
    if (sym) {
        printf("[MINIMAL TEST] symbol_new returned: %p\n", (void*)sym);
        if (sym->name) {
            printf("[MINIMAL TEST] sym->name: %s\n", sym->name);
        }
        symbol_free(sym);
    } else {
        printf("[MINIMAL TEST] symbol_new returned NULL\n");
    }
    return 0;
}
