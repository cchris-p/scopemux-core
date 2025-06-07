The goal of the context collector is to enable fast, intelligent interaction with a large codebase—especially for LLMs, search, summarization, and tooling. To that end, you should index only the *relevant, extractable, and useful* parts of the codebase that provide **semantic structure** and **operational intent**.

## ✅ **1. Function and Method Definitions**

- **Signature**
- **Start/end byte and line offsets**
- **Parameters and return types**
- **Enclosing class (if applicable)**
- **Modifiers** (static, const, inline, etc.)
- **Comments and docstrings above the function**

🔧 *Why:* Functions are the smallest reusable, callable units. They’re where logic lives and where questions often point (e.g., “What does `apply_mask()` do?”).

---

## ✅ **2. Class / Struct Definitions**

- **Name and location**
- **Inheritance / interfaces**
- **Member variables and types**
- **Access modifiers (public/private/protected)**
- **All method declarations**
- **Docstrings or comments at the class level**

🔧 *Why:* Classes are containers of behavior and structure. They define relationships and patterns in OO systems.

---

## ✅ **3. Global Constants and Macros**

- `#define`, `constexpr`, `const T name = ...`
- Capture their **values**, **types**, **comments**, and **location**

🔧 *Why:* These often encode fixed configuration values or key invariants used across multiple files.

---

## ✅ **4. Enums and Typedefs**

- Name, underlying type, member values
- Typedefs and `using` aliases

🔧 *Why:* These establish program-specific types and can improve LLM understanding of type-dependent logic.

---

## ✅ **5. Include / Import Statements**

- `#include`, `import`, `use`, etc.
- Resolve relative includes to absolute paths when possible

🔧 *Why:* Builds the **include graph** or **module dependency tree**, which is essential for cross-file context building.

---

## ✅ **6. Comments & Docstrings**

- Per-function/class leading comments
- Inline comments for CFG edge annotation
- TODO/FIXME tags with locations

🔧 *Why:* These give intent, usage semantics, and edge cases that are hard to infer from code alone.

---

## ✅ **7. Control Flow Graph (CFG)**

For each function:

- Nodes = basic blocks
- Edges = branches (`if`, `while`, `switch`)
- Labelled with condition expressions and loop types

🔧 *Why:* This powers activity diagram generation, info-block summarization, and token budget-aware compression.

---

## ✅ **8. Call Graph Edges**

- Function A → Function B (callsite recorded)
- Capture argument structure if possible

🔧 *Why:* Helps upstream/downstream traceability, especially in “impact analysis” use cases.

---

## ✅ **9. Language-Specific Entities**

Examples:

- **Python**: decorators, `async def`, `@dataclass`
- **C++**: templates, virtual methods, friend classes
- **Rust**: traits, impl blocks, lifetimes
- **JavaScript**: closures, arrow functions, module exports

🔧 *Why:* These add nuance to how behavior is defined and invoked.

---

## 🚫 **What NOT to Index**

- Raw comments that aren’t near a code entity
- `#if 0`/dead code blocks
- Code inside `.gitignore`d files (unless explicitly allowed)
- Build artifacts, vendored dependencies, test snapshots
- Inline temporary variables unless used in expressions

---

## Bonus: Tier Assignment

You can also assign **tiers** during indexing to aid later compression:

| Tier | Meaning | Examples |
| --- | --- | --- |
| 0 | Line-level | Single token span or variable |
| 1 | Function/method | A single named callable unit |
| 2 | File | All symbols within a file |
| 3 | Module / Folder | Package or directory scope |
| 4 | Project-wide | Top-level graph of the entire repo |

---

Would you like a JSON or FlatBuffer schema example for storing this info in your IR?