# 06-25-2025 - API Refactor TODO

Perfect! I'll create a comprehensive TODO list based on our analysis and architectural planning. Here's an actionable implementation roadmap:

# Tree-sitter Integration Refactor - TODO List
## 06-21-2025 - Critical Technical Debt Resolution

---

## 🚨 **PHASE 1: EMERGENCY EXTRACTION** (Critical Priority) ✅ **MOSTLY COMPLETE**
**Goal: Remove test pollution from production code**

### 1.1 Extract Test-Specific Logic ✅ **COMPLETE**
- [x] **Create** `core/src/processors/test_processor.c` ✅
- [x] **Extract** `is_hello_world_test` logic and similar special cases ✅
- [x] **Create** test processor header `core/include/scopemux/processors/test_processor.h` ✅
- [x] **Move** all test environment detection logic from `tree_sitter_integration.c` ✅
- [x] **Remove** `SCOPEMUX_RUNNING_C_EXAMPLE_TESTS` checks from core parser ✅

### 1.2 Extract Post-Processing Logic ⚠️ **PARTIALLY COMPLETE**
- [x] **Create** `core/src/processors/ast_post_processor.c` ✅
- [x] **Extract** node ordering/categorization logic ✅
- [x] **Extract** memory cleanup and node removal logic ✅
- [x] **Create** header `core/include/scopemux/processors/ast_post_processor.h` ✅
- [x] **Move** the massive 200+ line post-processing section from `ts_tree_to_ast()` ✅ **Function calls are integrated**
- [ ] **Complete extraction** - Main file still contains large inline post-processing logic 🔄 **IN PROGRESS**

### 1.3 Extract Docstring Processing ✅ **COMPLETE**
- [x] **Create** `core/src/processors/docstring_processor.c` ✅
- [x] **Extract** JavaDoc comment processing (`extract_doc_comment()`) ✅
- [x] **Extract** all docstring association logic (DocstringInfo struct, etc.) ✅
- [x] **Create** header `core/include/scopemux/processors/docstring_processor.h` ✅
- [x] **Remove** inline docstring handling from main AST building ✅ **Using process_docstrings() call**

---

## 🏗️ **PHASE 2: CORE FUNCTION DECOMPOSITION** (High Priority)
**Goal: Break down monolithic 400+ line functions**

### 2.1 Decompose `ts_tree_to_ast()` Function
- [ ] **Create** `create_ast_root()` - Root node creation logic
- [ ] **Create** `process_all_queries()` - Query processing coordination
- [ ] **Create** `apply_qualified_naming()` - Qualified name generation  
- [ ] **Create** `register_ast_with_context()` - Context registration logic
- [ ] **Reduce** main `ts_tree_to_ast()` to <50 lines of coordination code

### 2.2 Decompose `process_query_matches()` Function  
- [ ] **Create** `extract_node_info_from_captures()` - Capture processing
- [ ] **Create** `create_ast_node_from_match()` - Node creation logic
- [ ] **Create** `establish_node_relationships()` - Parent-child setup
- [ ] **Create** `populate_node_attributes()` - Signature, docstring, content setting
- [ ] **Reduce** main `process_query_matches()` to <50 lines

### 2.3 Extract Helper Functions
- [ ] **Move** `extract_full_signature()` to language-specific adapter
- [ ] **Move** `generate_qualified_name()` to dedicated naming module
- [ ] **Move** `extract_raw_content()` to node utilities module
- [ ] **Clean up** hardcoded capture name mapping logic

---

## 🔧 **PHASE 3: DIRECTORY STRUCTURE SETUP** (Medium Priority) ⚠️ **PARTIALLY COMPLETE**
**Goal: Establish scalable architecture foundation**

### 3.1 Create Core Directory Structure ⚠️ **PARTIALLY COMPLETE**
- [ ] **Create** `core/src/adapters/` directory
- [x] **Create** `core/src/processors/` directory ✅
- [ ] **Create** `core/src/config/` directory
- [ ] **Create** `core/include/scopemux/adapters/` directory
- [x] **Create** `core/include/scopemux/processors/` directory ✅
- [ ] **Create** `core/include/scopemux/config/` directory
- [ ] **Create** `core/config/` directory for JSON configuration files

### 3.2 Create Configuration Structure
- [ ] **Create** `core/config/languages/` directory
- [ ] **Create** `core/config/contexts/` directory  
- [ ] **Create** `core/config/processors/` directory
- [ ] **Create** `core/config/pipelines/` directory

---

## 🎯 **PHASE 4: LANGUAGE ADAPTER LAYER** (Medium Priority)
**Goal: Separate language-specific processing**

### 4.1 Create Base Adapter Interface
- [ ] **Create** `core/include/scopemux/adapters/language_adapter.h`
- [ ] **Define** standard adapter interface with function pointers
- [ ] **Create** `core/src/adapters/language_adapter.c` base implementation

### 4.2 Create Language-Specific Adapters
- [ ] **Create** `core/src/adapters/c_adapter.c` 
- [ ] **Move** C-specific signature extraction (`extract_full_signature`)
- [ ] **Move** C-specific node type mappings
- [ ] **Create** `core/src/adapters/python_adapter.c` (skeleton)
- [ ] **Create** `core/src/adapters/javascript_adapter.c` (skeleton)
- [ ] **Create** adapter headers for each language

### 4.3 Create Adapter Registry
- [ ] **Create** `core/src/adapters/adapter_registry.c`
- [ ] **Implement** dynamic adapter loading by language
- [ ] **Create** registration system for new adapters
- [ ] **Create** `core/include/scopemux/adapters/adapter_registry.h`

---

## ⚙️ **PHASE 5: PROCESSOR PIPELINE SYSTEM** (Medium Priority)  
**Goal: Create configurable processing pipeline**

### 5.1 Create Base Processor Interface
- [ ] **Create** `core/include/scopemux/processors/base_processor.h`
- [ ] **Define** standard processor interface  
- [ ] **Create** `core/src/processors/base_processor.c`

### 5.2 Create Specific Processors
- [ ] **Create** `core/src/processors/production_processor.c`
- [ ] **Create** `core/src/processors/validation_processor.c`
- [ ] **Create** `core/src/processors/node_ordering_processor.c`
- [ ] **Create** corresponding headers for each processor

### 5.3 Create Processing Pipeline
- [ ] **Create** `core/src/processors/processor_chain.c`
- [ ] **Implement** configurable processor pipeline execution
- [ ] **Add** processor configuration loading
- [ ] **Create** `core/include/scopemux/processors/processor_chain.h`

---

## 📋 **PHASE 6: CONFIGURATION SYSTEM** (Low Priority)
**Goal: Replace hardcoded logic with data-driven behavior**

### 6.1 Create Configuration Infrastructure  
- [ ] **Create** `core/src/config/config_loader.c`
- [ ] **Implement** JSON configuration file loading
- [ ] **Create** `core/src/config/language_config.c`
- [ ] **Create** `core/src/config/context_config.c`
- [ ] **Create** corresponding headers

### 6.2 Create Configuration Files
- [ ] **Create** `core/config/languages/c.json` 
- [ ] **Create** `core/config/contexts/basic_tests.json`
- [ ] **Create** `core/config/contexts/example_tests.json`
- [ ] **Create** `core/config/processors/docstring_rules.json`
- [ ] **Create** `core/config/pipelines/c_testing.json`

### 6.3 Replace Hardcoded Logic
- [ ] **Replace** hardcoded node type mappings with configuration
- [ ] **Replace** hardcoded capture name logic with configuration  
- [ ] **Replace** test environment detection with configuration
- [ ] **Remove** all remaining hardcoded strings and magic values

---

## 🧪 **PHASE 7: REFACTOR CORE PARSER** (Low Priority)
**Goal: Clean up the core parsing logic**

### 7.1 Simplify Core Integration
- [ ] **Rename** `tree_sitter_integration.c` to `tree_sitter_core.c`
- [ ] **Remove** all special case logic (moved to processors)
- [ ] **Remove** all test-specific code (moved to test processor)
- [ ] **Simplify** to pure Tree-sitter integration only
- [ ] **Update** `core/include/scopemux/tree_sitter_integration.h`

### 7.2 Create Clean AST Builder
- [ ] **Create** `core/src/parser/ast_builder.c`
- [ ] **Move** core AST construction logic from integration file
- [ ] **Remove** language-specific logic (moved to adapters)
- [ ] **Create** `core/include/scopemux/parser/ast_builder.h`

### 7.3 Update Main Parser Interface
- [ ] **Update** `core/src/parser/parser.c` to use new architecture
- [ ] **Integrate** adapter registry and processor chain
- [ ] **Update** public API to maintain backward compatibility
- [ ] **Update** `core/include/scopemux/parser.h`

---

## 🔄 **PHASE 8: BUILD SYSTEM & TESTING** (Low Priority)
**Goal: Ensure new architecture builds and tests pass**

### 8.1 Update Build Configuration
- [ ] **Update** `core/CMakeLists.txt` with new source files
- [ ] **Add** new directories to build system
- [ ] **Update** include paths for new header structure
- [ ] **Add** configuration file installation

### 8.2 Update Tests  
- [ ] **Update** existing tests to work with new architecture
- [ ] **Create** unit tests for individual adapters
- [ ] **Create** unit tests for individual processors
- [ ] **Verify** all existing test cases still pass

---

## ✅ **SUCCESS CRITERIA**

### Code Quality Metrics
- [ ] **No function** over 100 lines
- [ ] **No file** over 500 lines  
- [ ] **Zero hardcoded** test logic in production code
- [ ] **All special cases** moved to configuration or processors

### Architecture Validation
- [ ] **New language** can be added with just adapter + config
- [ ] **New test context** can be added with just processor + config  
- [ ] **Core parser** has zero test-specific logic
- [ ] **All processors** are independently testable

### Performance & Maintainability
- [ ] **Parsing performance** maintained or improved
- [ ] **Memory usage** maintained or reduced
- [ ] **Code coverage** maintained above 80%
- [ ] **Documentation** updated for new architecture

---

**Estimated Timeline**: 2-3 weeks
**Risk Level**: Medium (significant refactor but well-planned)
**Priority**: **HIGH** - Technical debt is becoming critical

Ready to start implementation? I recommend beginning with Phase 1 (Emergency Extraction) to immediately address the most critical technical debt.
