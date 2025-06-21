---
description: Directory Structure for the Tree Sitter API Refactor
---

<implementation_reference>
Directory changes described in: `api_refactor_notes.md`
</implementation_reference>

<filesystem>
core/
└── src/
    ├── parser/
    │   ├── tree_sitter_core.c
    │   ├── ast_builder.c
    │   └── ...
    ├── adapters/
    │   ├── language_adapter.c
    │   ├── c_adapter.c
    │   └── ...
    ├── processors/
    │   ├── base_processor.c
    │   ├── test_processor.c
    │   ├── ast_post_processor.c
    │   └── processor_chain.c
    └── config/
        ├── config_loader.c
        └── ...
include/scopemux/ (mirrors src layout)
config/
├── languages/c.json
├── contexts/basic_tests.json
└── pipelines/c_testing.json
</filesystem>