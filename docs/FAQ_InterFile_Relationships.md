# How `parser.h` Structures the Abstract Syntax Tree and Supports Relationships

The `parser.h` file gives us a clear picture of how the **Abstract Syntax Tree (AST)** and **Concrete Syntax Tree (CST)** are structured and how relationships—especially inter-file ones—can be preserved.

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

* `qualified_name` is crucial for **cross-file resolution**.
* `references` holds symbolic links to **other ASTNodes**, forming the basis for linking usages to definitions.

***

### **ParserContext Structure**

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

* `all_nodes` enables fast lookup and traversal during linking/resolution.
* The context now supports both AST and CST representations.

***

### 🔧 Key Functions for Building Relationships

* `ASTNode *ast_node_new(...)`

  Creates a new `ASTNode`.

* `bool ast_node_add_child(ASTNode *parent, ASTNode *child)`

  Establishes a **parent-child** relationship (intra-file structure).

* `bool ast_node_add_reference(ASTNode *from, ASTNode *to)`

  Records a **semantic reference** from one node to another.

  > Used to link a call-site ASTNode to a definition ASTNode, even across files.

***

### 🔁 Inter-File Relationship Workflow

### Step 1: Parse Each File

Each source file is parsed into its own `ParserContext`, producing both AST and CST trees.

### Step 2: Build a Global Symbol Table

* Iterate over all `ParserContext`s.
* Construct a **global map** from `qualified_name → ASTNode*`.
* This map serves as the cross-file symbol directory.

### Step 3: Resolve and Link References

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

### 🌱 Role of `tree_sitter_integration.c`

* Responsible for **generating the AST and CST** per file using Tree-sitter queries.
* The `ts_tree_to_ast` function uses Tree-sitter queries to extract semantic information.
* The `ts_tree_to_cst` function creates a full-fidelity syntax tree with every token.
* Captures **potential reference points** (e.g., function call expressions).
* **Does not resolve** references—that's left to another stage of ScopeMux, using the global symbol table and `ast_node_add_reference`.

***

### ✅ Conclusion

This dual AST/CST design provides a solid foundation for **representing and resolving cross-file dependencies**:

* The `ASTNode.references` field is precisely what's needed for symbolic linkage.
* The global `qualified_name → ASTNode` map acts as the lookup backbone.
* The CST provides complete syntactic information when needed for formatting or reconstruction.

***

# *How will relationships between classes, variables and functions be preserved when considering one file calls on another file's stuff?*

### 1. Individual File Parsing

Each file is first parsed independently. The `tree_sitter_integration.c` module, using `ParserContext`, generates both **Abstract Syntax Tree (AST)** and **Concrete Syntax Tree (CST)** representations for each file.

At this stage:

* An `ASTNode` for a function call like `other_file_function()` in `current_file.c` knows that it's a call to a function named `other_file_function`.
* But it **doesn't yet know where that function is defined**. It's just a name at this point.

***

### 2. Symbol Table Generation

After (or during) the parsing of all relevant files in a project, a **symbol table** (or multiple tables) is usually constructed.

This symbol table acts like a **directory**, mapping symbol names (like function names, class names, variable names) to their actual definitions (i.e., the specific `ASTNode` that represents their declaration in whichever file they are defined).

For example:

* The symbol table would record that `other_file_function` is defined in `other_file.c` at a particular `ASTNode`.

***

### 3. Linking/Resolution Phase

Once the symbol table(s) are populated, a **linking** or **resolution** phase occurs.

* The system revisits the AST for each file.
* When it encounters a usage of a symbol (like the call to `other_file_function()`), it **looks up that symbol's name** in the symbol table.
* If found, the `ASTNode` representing the usage can then be updated to **store a direct reference** (e.g., a pointer or an ID) to the `ASTNode` representing the definition of `other_file_function()` from `other_file.c`.

***

### 4. Storing Relationships in the AST

The `ASTNode` structure itself is designed to store these **resolved links**.

Examples:

* An `ASTNode` for a function call might have a field like `resolved_target_function` that points to the `ASTNode` of the actual function being called.
* For class inheritance, a class `ASTNode` might store references to its **parent class's `ASTNode`**.
* Variable usages would link to their **declaration `ASTNode`s**.

***

### Summary for ScopeMux

* The `tree_sitter_integration.c` part handles **step 1** (individual file parsing) through `ts_tree_to_ast` and `ts_tree_to_cst`.
* To achieve what you're asking, ScopeMux would need **additional logic** (likely in a different module or at a higher level of orchestration) to perform **steps 2 and 3**.
* This could involve a **project analysis** component that manages multiple `ParserContext`s and builds a **global (or per-scope) understanding**.
* The `ASTNode` structure would be **key to storing these resolved cross-file relationships**.
* The current focus of ScopeMux has been on getting the **single-file parsing and AST/CST generation correct** using Tree-sitter queries.

  The **inter-file linking** is a **subsequent, more complex layer** that builds upon this foundation.
