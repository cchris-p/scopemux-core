# Reference Resolution Refactor TODO

This document tracks the concrete steps required to implement and migrate to the new generalized reference resolution architecture in ScopeMux.

---

## Core Implementation

- [ ] Create `reference_resolution_core.c` and `reference_resolution_core.h` in `core/src/parser/reference_resolvers/`.
- [ ] Implement recursive qualified name resolution (multi-segment names).
- [ ] Implement hierarchical scope walking (local, enclosing, global).
- [ ] Provide language hook struct (`LanguageResolverHooks`) for import/include and attribute/member lookup.
- [ ] Implement centralized logging and statistics collection.
- [ ] Provide helpers for reference type inference.
- [ ] Remove any builtin fallback logic from the core (builtin fallback is not supported).

---

## Language Resolver Migration

- [ ] Refactor `python_resolver.c` to delegate to the core for all qualified name and scope resolution.
- [ ] Refactor `c_resolver.c`, `cpp_resolver.c`, `javascript_resolver.c`, and `typescript_resolver.c` similarly.
- [ ] Remove all direct builtin fallback logic from every resolver.
- [ ] Replace all old or inconsistent names/enums with canonical versions in all files, headers, and tests.
- [ ] Update all documentation and code comments to reflect new architecture and naming.

---

## Tests and Validation

- [ ] Add/expand tests for the core in `core/tests/`:
    - `reference_resolver_tests.c`
    - `language_resolver_tests.c`
    - Add new cross-language/core API tests as needed.
- [ ] Ensure all golden files and test goldens are updated for new naming and resolver behavior.
- [ ] Validate statistics and logging output for all supported languages.

---

## Finalization

- [ ] Review and cleanup legacy logic and dead code.
- [ ] Ensure all language hooks are documented and extensible.
- [ ] Document migration steps for adding new languages or extending reference resolution behavior.
