# **ScopeMux Refactoring Implementation Plan (Post Phase 1 Completion)**

## **1. Complete Phase 3: Directory Structure Setup**

### **1.1 Create Core Directory Structure**
```bash
# Create adapter directories
mkdir -p core/src/adapters/
mkdir -p core/include/scopemux/adapters/

# Create config directories  
mkdir -p core/src/config/
mkdir -p core/include/scopemux/config/
mkdir -p core/config/languages/
mkdir -p core/config/contexts/
mkdir -p core/config/processors/
mkdir -p core/config/pipelines/
```

### **1.2 Add Placeholder Files**
```bash
# Adapter base files
touch core/include/scopemux/adapters/language_adapter.h
touch core/src/adapters/language_adapter.c

# Config base files  
touch core/include/scopemux/config/config_loader.h
touch core/src/config/config_loader.c
```

### **1.3 Update CMakeLists.txt**
Add new directories to build system:
```cmake
# Add to core/CMakeLists.txt
add_subdirectory(src/adapters)
add_subdirectory(src/config)
```

## **2. Begin Phase 4: Language Adapter Layer**

### **2.1 Create Base Adapter Interface**
File: `core/include/scopemux/adapters/language_adapter.h`
```c
typedef struct {
    // Language identification
    LanguageType language_type;
    const char *language_name;
    
    // Processing functions
    char* (*extract_signature)(TSNode node, const char* source_code);
    char* (*generate_qualified_name)(const char* name, ASTNode* parent);
    void (*process_special_cases)(ASTNode* node, ParserContext* ctx);
} LanguageAdapter;
```

### **2.2 Create C Language Adapter**
File: `core/src/adapters/c_adapter.c`
```c
#include "scopemux/adapters/language_adapter.h"

static char* c_extract_signature(TSNode node, const char* source_code) {
    // Move extract_full_signature() logic here
}

LanguageAdapter c_adapter = {
    .language_type = LANG_C,
    .language_name = "C",
    .extract_signature = c_extract_signature,
    // Other C-specific implementations
};
```

### **2.3 Create Adapter Registry**
File: `core/src/adapters/adapter_registry.c`
```c
#include "scopemux/adapters/language_adapter.h"

LanguageAdapter* registered_adapters[MAX_ADAPTERS];

void register_adapter(LanguageAdapter* adapter) {
    // Implementation
}

LanguageAdapter* get_adapter(LanguageType lang) {
    // Implementation
}
```

## **3. Continue Phase 2: Decompose process_query_matches()**

### **3.1 Extract Core Functions**
From `tree_sitter_integration.c`:
```c
// New function 1: Capture processing
static void process_captures(TSQueryMatch match, ParserContext* ctx, 
                           char** node_name, uint32_t* node_type) {
    // Extract lines 305-347
}

// New function 2: Node creation  
static ASTNode* create_ast_node_from_match(ParserContext* ctx, TSNode target_node,
                                         const char* node_name, uint32_t node_type) {
    // Extract lines 350-400
}

// New function 3: Relationship establishment
static void establish_node_relationships(ASTNode* node, ASTNode** node_map, 
                                       ASTNode* parent_node) {
    // Extract lines 405-430
}
```

### **3.2 Refactored process_query_matches()**
```c
static void process_query_matches(...) {
    while (ts_query_cursor_next_match(cursor, &match)) {
        // 1. Process captures
        process_captures(match, ctx, &node_name, &node_type);
        
        // 2. Create node  
        ASTNode* ast_node = create_ast_node_from_match(ctx, target_node, node_name, node_type);
        
        // 3. Establish relationships
        establish_node_relationships(ast_node, node_map, parent_node);
    }
}
```

## **Implementation Order**

1. **First**: Complete Phase 3 directory structure
2. **Then**: Implement base adapter interface (Phase 4)
3. **Finally**: Decompose process_query_matches() (Phase 2)

## **Verification Steps**

1. **Build System Test**:
```bash
cd core/ && cmake . && make
```

2. **Unit Tests**:
```bash
./run_c_tests.sh
./run_misc_tests.sh
```

3. **Memory Checks**:
```bash
valgrind ./run_c_tests.sh
```

