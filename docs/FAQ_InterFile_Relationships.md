# FAQ: Inter-File Relationships in ScopeMux
## Current Implementation vs Planned Features

This FAQ addresses how `parser.h` structures the **Abstract Syntax Tree (AST)** and **Concrete Syntax Tree (CST)**, and explains both the **current capabilities** and **planned features** for inter-file relationships.

## Status Legend
- ✅ **Implemented**: Feature is working in current codebase
- 📋 **Planned**: Feature is designed but not yet implemented
- ❓ **FAQ**: Frequently asked questions about current/future capabilities

***

### 🧱 Key Takeaways

### **ASTNode Structure**

```c
typedef struct ASTNode {
    ASTNodeType type;                 // What the node represents (function, class, etc.)
    char *name;                       // Simple name of the entity
    char *qualified_name;             // Fully-qualified name (e.g., namespace::class::method)
    SourceRange range;                // Location in the source file
    struct ASTNode *parent;           // Parent ASTNode within the same file
    struct ASTNode **children;        // Array of child ASTNodes
    size_t num_children;              // Number of children
    struct ASTNode **references;      // References to other ASTNodes (inter/intra-file)
    size_t num_references;            // Number of references
    void *additional_data;            // Optional extra metadata or analysis results
} ASTNode;

```

* `qualified_name` is crucial for **cross-file resolution** (✅ field exists, 📋 cross-file resolution planned).
* `references` holds symbolic links to **other ASTNodes** (✅ field exists, 📋 cross-file linking planned).

***

### **ParserContext Structure (✅ Implemented)**

```c
typedef struct ParserContext {
    ASTNode *ast_root;               // Root of the AST for a single file
    CSTNode *cst_root;               // Root of the CST for a single file
    ASTNode **all_nodes;             // Flat list of all ASTNodes in the file
    TSParser *ts_parser;             // Tree-sitter parser instance
    const char *source_code;         // Source code being parsed
    QueryManager *q_manager;         // Manager for Tree-sitter queries
    // Other fields...
} ParserContext;

```

* `all_nodes` enables fast lookup and traversal (✅ field exists, 📋 cross-file lookup planned).
* The context supports both AST and CST representations (✅ implemented).

***

### 🔧 Key Functions for Building Relationships (✅ Implemented)

* `ASTNode *ast_node_new(...)` ✅

  Creates a new `ASTNode`.

* `bool ast_node_add_child(ASTNode *parent, ASTNode *child)` ✅

  Establishes a **parent-child** relationship (intra-file structure).

* `bool ast_node_add_reference(ASTNode *from, ASTNode *to)` ✅

  Records a **semantic reference** from one node to another.

  > Used to link a call-site ASTNode to a definition ASTNode (✅ function exists, 📋 cross-file usage planned).

***

### 🔁 Inter-File Relationship Workflow (📋 Planned)

### Step 1: Parse Each File (✅ Implemented)

Each source file is parsed into its own `ParserContext`, producing both AST and CST trees.

### Step 2: Build a Global Symbol Table (📋 Planned)

* Iterate over all `ParserContext`s.
* Construct a **global map** from `qualified_name → ASTNode*`.
* This map serves as the cross-file symbol directory.

### Step 3: Resolve and Link References (📋 Planned)

For each `ASTNode`:

1. Identify references (e.g., a call to `foo()`).
2. Derive its `qualified_name`.
3. Look it up in the global symbol map.
4. If found, call:

   ```c
   ast_node_add_reference(calling_node, target_foo_node);

   ```

   to establish the link.

***

### 🌱 Role of `tree_sitter_integration.c` (✅ Implemented)

* Responsible for **generating the AST and CST** per file using Tree-sitter queries ✅.
* The `ts_tree_to_ast` function uses Tree-sitter queries to extract semantic information ✅.
* The `ts_tree_to_cst` function creates a full-fidelity syntax tree with every token ✅.
* Captures **potential reference points** (e.g., function call expressions) ✅.
* **Does not resolve** references—that's left to another stage of ScopeMux (📋 planned), using the global symbol table and `ast_node_add_reference`.

***

### ✅ Conclusion

This dual AST/CST design provides a solid foundation for **representing and resolving cross-file dependencies**:

* The `ASTNode.references` field is precisely what's needed for symbolic linkage.
* The global `qualified_name → ASTNode` map acts as the lookup backbone.
* The CST provides complete syntactic information when needed for formatting or reconstruction.

***

# ❓ FAQ: *How will relationships between classes, variables and functions be preserved when considering one file calls on another file's stuff?*

### 1. Individual File Parsing (✅ Implemented)

Each file is first parsed independently. The `tree_sitter_integration.c` module, using `ParserContext`, generates both **Abstract Syntax Tree (AST)** and **Concrete Syntax Tree (CST)** representations for each file.

At this stage:

* An `ASTNode` for a function call like `other_file_function()` in `current_file.c` knows that it's a call to a function named `other_file_function`.
* But it **doesn't yet know where that function is defined**. It's just a name at this point.

***

### 2. Symbol Table Generation (📋 Planned)

After (or during) the parsing of all relevant files in a project, a **symbol table** (or multiple tables) is usually constructed.

This symbol table acts like a **directory**, mapping symbol names (like function names, class names, variable names) to their actual definitions (i.e., the specific `ASTNode` that represents their declaration in whichever file they are defined).

For example:

* The symbol table would record that `other_file_function` is defined in `other_file.c` at a particular `ASTNode`.

***

### 3. Linking/Resolution Phase (📋 Planned)

Once the symbol table(s) are populated, a **linking** or **resolution** phase occurs.

* The system revisits the AST for each file.
* When it encounters a usage of a symbol (like the call to `other_file_function()`), it **looks up that symbol's name** in the symbol table.
* If found, the `ASTNode` representing the usage can then be updated to **store a direct reference** (e.g., a pointer or an ID) to the `ASTNode` representing the definition of `other_file_function()` from `other_file.c`.

***

### 4. Storing Relationships in the AST (✅ Structure Ready, 📋 Cross-file Usage Planned)

The `ASTNode` structure itself is designed to store these **resolved links**.

Examples:

* An `ASTNode` for a function call might have a field like `resolved_target_function` that points to the `ASTNode` of the actual function being called.
* For class inheritance, a class `ASTNode` might store references to its **parent class's `ASTNode`**.
* Variable usages would link to their **declaration `ASTNode`s**.

***

### Summary for ScopeMux

* The `tree_sitter_integration.c` part handles **step 1** (individual file parsing) through `ts_tree_to_ast` and `ts_tree_to_cst` ✅.
* To achieve what you're asking, ScopeMux would need **additional logic** (likely in a different module or at a higher level of orchestration) to perform **steps 2 and 3** 📋.
* This could involve a **project analysis** component that manages multiple `ParserContext`s and builds a **global (or per-scope) understanding** 📋.
* The `ASTNode` structure would be **key to storing these resolved cross-file relationships** (✅ structure ready, 📋 cross-file usage planned).
* The current focus of ScopeMux has been on getting the **single-file parsing and AST/CST generation correct** using Tree-sitter queries ✅.

  The **inter-file linking** is a **subsequent, more complex layer** that builds upon this foundation 📋.

## 📊 **FEATURE COMPLETION MEASUREMENT**

### Quick Status Assessment
**Current Implementation: 20% Complete**

### How to Calculate Completion Percentage

**Phase Weightings:**
- Phase 1 (Foundation): 20% of total ✅ **COMPLETE**
- Phase 2 (Multi-File Infrastructure): 25% of total 📋 **0% done**
- Phase 3 (Reference Resolution): 30% of total 📋 **0% done**  
- Phase 4 (Integration & Enhancement): 20% of total 📋 **0% done**
- Phase 5 (Advanced Features): 5% of total 📋 **0% done**

### For Developers/LLM Agents: Quick Assessment Checklist

**Phase 1: Foundation (✅ 20% Complete)**
- ✅ Can parse individual C/Python/JS files
- ✅ ASTNode structure has `references` and `qualified_name` fields
- ✅ Tree-sitter integration working
- ✅ Memory management functional

**Phase 2: Multi-File Infrastructure (📋 0% Complete)**
- [ ] ProjectContext structure exists
- [ ] GlobalSymbolTable implemented  
- [ ] Can parse multiple files in one project
- [ ] Symbol registration across files works

**Phase 3: Reference Resolution (📋 0% Complete)**
- [ ] Function calls resolved across files
- [ ] Import/include statements processed
- [ ] Cross-file class inheritance working
- [ ] Symbol lookup algorithms functional

**Phase 4: Integration (📋 0% Complete)**
- [ ] Call graphs span multiple files
- [ ] InfoBlocks work across files
- [ ] TieredContexts include cross-file data
- [ ] All APIs implemented

**Phase 5: Advanced Features (📋 0% Complete)**
- [ ] Type inference across files
- [ ] Performance optimized for large projects
- [ ] Language server integration

### Testing Commands
```bash
# Test current capabilities (Phase 1)
./run_misc_tests.sh
./run_c_tests.sh
./run_python_tests.sh

# TODO: Multi-file tests (Phase 2+)
# (Tests don't exist yet for cross-file functionality)
```

### Success Metrics Targets
- **Symbol Resolution Rate**: >95% (Current: 0%)
- **Language Coverage**: C/C++, Python, JS/TS (Current: 0 languages cross-file)
- **Performance**: 1000 files in <30 seconds (Current: Not applicable)

## 🎯 **Current Status Summary**
- ✅ **Foundation Complete**: All basic infrastructure for inter-file relationships is in place
- 📋 **Next Steps**: Multi-file infrastructure, symbol tables, and cross-file resolution
- ❓ **Detailed Metrics**: See [Inter-File Relationships Design Specification](./Inter-File%20Relationships%20for%20ASTNodes.md#-completion-measurement-guidelines) for complete implementation roadmap and detailed completion criteria
