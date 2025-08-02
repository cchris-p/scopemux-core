1. There are way to many node mapping print statements, which led me to think are there redundancies amongst node mappings? Is the source of truth actually being referenced in the queries / ast building?

matrillo@matrillosub1:~/apps/scopemux-core$ grepx --exclude="*.txt" "Loading hardcoded node type mappings"
core/src/parser/ts_init.c:    log_info("Loading hardcoded node type mappings (source of truth)...");
core/src/config/node_type_mapping_loader.c:  fprintf(stderr, "[scopemux] INFO: Loading hardcoded node type mappings\n");
core/src/config/node_type_mapping_loader.c:  printf("[scopemux] Loading hardcoded node type mappings:\n");
grep: scripts/parse_cst: binary file matches
grep: scripts/parse_ast: binary file matches
actual_hello_world_ast.json:[scopemux] Loading hardcoded node type mappings:
grep: build-c/core/libparser_core.a: binary file matches
matrillo@matrillosub1:~/apps/scopemux-core$ 


In run_c_tests.txt:

[c Example Test: docstring_test.c] [2025-07-30 21:16:24] [INFO] Loading hardcoded node type mappings (source of truth)...
[c Example Test: docstring_test.c] [scopemux] INFO: Loading hardcoded node type mappings
[c Example Test: docstring_test.c] [scopemux] Loaded 14 hardcoded node type mappings


2. 