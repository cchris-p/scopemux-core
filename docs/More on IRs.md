# ScopeMux IRs: Conceptual Foundations, Relationships, and Advanced Topics

## 🧠 Core Philosophy

**IRs (Intermediate Representations)** are semantically rich abstractions *built on top of the AST*. They are meant to:

* **Encode meaning**: definitions, control flow, types, dependencies.
* **Drive views**: like UML diagrams, function call maps, or API summaries.
* **Feed TieredContexts**: as compressed, interpretable formats for downstream LLMs.

***

## 🧱 Conceptual Categories of IRs

Thinking about IRs in categories can help organize their purposes. While specific IRs are detailed in the "Supported IRs" document, general categories often include structural, relational/graph-based, and semantic IRs. This document explores the broader concepts and roles of IRs within ScopeMux.

***

## 🧩 Sample Flow: Illustrating IR Inter-relationships

IRs often work together. For example, a **UML Class Diagram** might be generated using:

* `Symbol IR` (for class/function defs)
* `Type IR` (for attribute/method types)
* `Import IR` (for external references)
* → transformed into a **TieredContext UML View**, which updates in real time.

This illustrates how different IRs contribute to a single, higher-level understanding or artifact.

***

## 🗺️ Future or Advanced IR Considerations

The ScopeMux IR framework is extensible. While the "Supported IRs" document lists currently implemented representations, this space can be used to discuss potential future IRs or more advanced concepts that could further enhance code understanding and manipulation capabilities. Examples previously considered here (like Semantic, Concurrency, Contract, and Regulatory IRs) have been integrated into the supported list as they matured.

***

## 💡 Summary Table of IR Concepts

This table provides a conceptual overview of various IR types and their general roles. For specific details on supported IRs, refer to the "Supported IRs" document.

| IR Concept Category | Feeds Into Examples | UML-Related? | Core Use Focus |
| --- | --- | --- | --- |
| Symbol/Definition Tracking | Class Diagrams, API maps | ✅ | Definition/use tracking |
| Call/Execution Flow | Sequence Diagrams | ✅ | Function/Method topology |
| Control/Activity Flow | Activity Diagrams | ✅ | Logic flow (CFG) |
| Type Systems | Attribute/method types in diagrams | ✅ | Type introspection |
| Module/Dependency Mapping | Component Diagrams | ✅ | Package/module relationships |
| Error/Exception Handling | Fault/Exception Diagrams | ✅ | Resilience mapping |
| Object/Memory Lifecycle | Object Diagrams | ✅ | Lifetime/model diagrams |
| Documentation Extraction | Class/API summaries | Partial | LLM doc views, code summaries |
| Runtime Behavior Tracing | Dynamic Sequence, Performance Diagrams | ✅ | Real-time behavior analysis |
| Test/Code Linkage | Test ↔ Code Tier Maps | Indirect | QA and safety views |

***

## 🔄 TL;DR: IRs vs. Tiered Contexts

* **IRs** are *raw semantic models* extracted from the AST.
* **Tiered Contexts** are *compressed, curated views* built **on top of one or more IRs**, tailored for specific use cases like:
  * LLM prompt injection
  * Diagrams
  * UI panes
  * Navigation tools

**IRs = structured knowledge graph**

**Tiered Contexts = human/LLM-consumable slices of that graph**

***

## 🧱 Why IRs are Foundational (and distinct from Tiered Contexts)

### ✅ 1. **IRs capture full structure and semantics**

IRs are meant to be **complete and lossless** representations of a specific semantic property of code (symbols, control flow, imports, etc.). Think of each IR like a normalized SQL table or a graph database: they aim for comprehensive capture of their domain.

Tiered Contexts, on the other hand, are selective views or aggregations *derived* from these rich IRs, optimized for specific tasks or token budgets. They don't aim for completeness in the same way; they aim for relevance and conciseness for a particular purpose.

* `SymbolIR`: every defined entity, its type, scope, and location.
* `CallGraphIR`: every call edge in the code.
* `ActivityIR`: every control flow path through methods.

They don’t care **who consumes them** yet — just that they’re **accurate and richly structured**.

***

### 🧰 2. **Tiered Contexts are views** (derived from IRs)

Once you’ve built your IRs, you selectively **compress, filter, or slice** them into **Tiered Contexts**, depending on the consumer:

| Consumer Type | Tiered Context Example |
| --- | --- |
| LLM | Function call map + class summaries + comments |
| Diagram (UML) | Class defs + types + relations from `SymbolIR` |
| IDE sidebar | Docstrings + definitions only |
| Autocomplete engine | Local `SymbolIR` for file + type data |
| Error checker | Control flow + exception IR |

Each Tiered Context answers:

**“What subset of IR data do I need to inject into this interface?”**

***

## 🧠 Design Choice Justification

### ✅ Keep IRs atomic and reusable:

> Let each IR solve one semantic concern and be independent.

* This lets you recompute just one IR when only a subset of the code changes.
* You can **cache**, **compose**, or **diff** IRs cheaply.

### ✅ Let Tiered Contexts remain dynamic:

> They're runtime, user-facing, task-aware views.

* You may have dozens of contexts built from the same IRs:
  * `"docstring-only context"` from `SymbolIR + DocIR`
  * `"test coverage heatmap"` from `SymbolIR + CoverageIR`
  * `"regime change prompt"` from `ActivityIR + ExceptionIR`

***

## 🎯 Analogy

### IRs are like:

* **SQL tables**
* **Compiler passes**
* **Normalized knowledge graphs**

### Tiered Contexts are like:

* **SQL views**
* **Frontend slices**
* **LLM input builders**

***

## ✅ Summary

| Concept | Role | Lifespan | Consumers |
| --- | --- | --- | --- |
| **IR** | Full structured semantic model | Persistent | Tier builder, visualizer |
| **Tiered Context** | Custom view for a purpose/task | Runtime/dynamic | LLM, GUI, debugger, IDE |

> 🧠 Key rule:
>
> **IRs are what you index and search.**
>
> **Tiered Contexts are what you show or feed into models.**

Would you like a sample TieredContext built from 2 IRs to illustrate this?

Great. Let’s walk through a concrete example: building a **TieredContext for UML Class Diagrams**, using two IRs: `SymbolIR` and `TypeIR`.

***

## 🧱 Step 1: IRs (the raw data)

### 🧩 `SymbolIR` (example)

```json
[
  {
    "kind": "class",
    "name": "Account",
    "location": "bank/models/account.py:5",
    "methods": ["__init__", "deposit", "withdraw"],
    "attributes": ["balance", "owner"]
  },
  {
    "kind": "function",
    "name": "transfer",
    "location": "bank/models/account.py:42",
    "calls": ["Account.withdraw", "Account.deposit"]
  }
]

```

### 🧩 `TypeIR` (example)

```json
[
  { "symbol": "Account.balance", "type": "float" },
  { "symbol": "Account.owner", "type": "User" },
  { "symbol": "Account.deposit", "args": ["float"], "return": "None" },
  { "symbol": "Account.withdraw", "args": ["float"], "return": "bool" },
  { "symbol": "transfer", "args": ["Account", "Account", "float"], "return": "bool" }
]

```

***

## 🧠 Step 2: Combine into a **TieredContext**

Now we synthesize a **TieredContext** that serves the purpose:

> "Generate a UML class diagram view of core banking models."

This TieredContext might look like:

```json
{
  "name": "uml_class_diagram:bank.models",
  "source_IRs": ["SymbolIR", "TypeIR"],
  "context_type": "uml_class_diagram",
  "entities": [
    {
      "type": "class",
      "name": "Account",
      "attributes": [
        { "name": "balance", "type": "float" },
        { "name": "owner", "type": "User" }
      ],
      "methods": [
        { "name": "__init__", "signature": "(self)" },
        { "name": "deposit", "signature": "(self, amount: float) -> None" },
        { "name": "withdraw", "signature": "(self, amount: float) -> bool" }
      ]
    }
  ],
  "relations": [
    {
      "from": "transfer",
      "to": "Account.deposit",
      "kind": "calls"
    },
    {
      "from": "transfer",
      "to": "Account.withdraw",
      "kind": "calls"
    }
  ]
}

```

This can be:

* Rendered into a live-updating **UML class diagram**
* Injected into an LLM as a "class skeleton"
* Used in the IDE’s “class browser” sidebar

***

## 🔁 Dynamic Update Flow

1. Code change (e.g., `Account.add_interest()` is added).
2. `SymbolIR` delta is recomputed incrementally.
3. `TypeIR` picks up type signature for the new method.
4. **TieredContext** re-renders diagram and syncs with WebSocket or LLM cache.

***

## 🔮 Bonus: Other TieredContext Types You Could Build

| Name | IRs Used | Purpose |
| --- | --- | --- |
| `prompt_context:class_summary` | SymbolIR + DocIR | Inject class names + docstrings into LLM |
| `coverage_context:untested_paths` | SymbolIR + TestCoverageIR | Show only untested methods |
| `activity_view:login_flow` | ActivityIR + ExceptionIR | Visualize flow of login logic |
| `api_surface:public_endpoints` | SymbolIR + ImportIR + DocIR | List public API functions/modules |

***

## ✅ Summary

* The **IRs** are structured, comprehensive, reusable.
* The **TieredContext** is a *filtered, composed*, and *purpose-driven* view derived from IRs.
* One TieredContext might combine **many IRs**, and can be:
  * Rendered visually
  * Used for navigation
  * Injected into LLMs

Would you like code to generate this TieredContext automatically from JSON IR inputs?

Would this be InfoBlocks, TieredContexts or IR?

A selection of multiple functions pprovided in a file and usage in a class of another file

A selection of all docstrings in a directory

A selection of all functions and all public variables (consts, enums, ints, etc) (exccluding classes) in a directory

Great distinctions. Let's classify each of your examples across the **three layers** in your system:

***

| Example | Classification | Explanation |
| --- | --- | --- |
| **1. A selection of multiple functions provided in a file and usage in a class of another file** | ✅ **InfoBlocks**⬇️ TieredContext (derived) | The *functions + usage mapping* comes from **SymbolIR + CallGraphIR**, but the *selection* (user-picked or auto-picked chunk) makes it an **InfoBlock**. If you convert it into a reusable format (e.g., show "function providers + usage summary"), it becomes a TieredContext. |
| **2. A selection of all docstrings in a directory** | ✅ **TieredContext** | Derived from **DocIR + SymbolIR**, this is a classic TieredContext: it's a filtered view optimized for LLMs or documentation panels. |
| **3. A selection of all functions and public variables (excluding classes) in a directory** | ✅ **TieredContext** | This is a *curated view* derived from **SymbolIR**, scoped to certain definitions and visibility rules. Could feed LLMs, IDE panes, etc. |

***

### 🧠 Deeper Breakdown

| Layer | Role | What it does in your examples |
| --- | --- | --- |
| **IR** | Raw, structured semantic data | Provides all symbols, docstrings, call links |
| **InfoBlock** | Focused, semantically coherent *chunk* | A user-selected group of function defs, e.g., *“all `email_*` helpers”* |
| **TieredContext** | Task-optimized view assembled from IR or InfoBlocks | “All docstrings”, “Public API Summary”, “Diagram View” |

***

### 🔁 Pipeline Mapping (Example 1)

**Use Case**: *“Give me a view of 3 helper functions and where they’re used.”*

1. `SymbolIR`: gives function definitions
2. `CallGraphIR`: shows usage in another class
3. ➡ You group these into an **InfoBlock** (user- or rule-defined)
4. ➡ You create a **TieredContext** from it:
   * A “HelperFunction → Usage Map”
   * Or a collapsible sidebar section
   * Or an LLM code prompt

***

### ✅ Summary by Intent

| Your Input | IRs Involved | InfoBlock? | TieredContext? |
| --- | --- | --- | --- |
| Functions + usage cross-files | SymbolIR + CallGraphIR | ✅ Yes | ✅ Yes (if turned into a view) |
| All docstrings in a directory | DocIR + SymbolIR | ❌ | ✅ Yes |
| All functions + constants/enums (no classes) in directory | SymbolIR | ❌ | ✅ Yes |

Would you like a CLI prototype that builds these TieredContexts from InfoBlocks in a directory?

Can a TieredContext be a selection of a uml diagram, some info blocks, and like three files?

Yes — a **TieredContext can absolutely include a UML diagram, InfoBlocks, and raw file content** — that's actually a *powerful and valid design choice*. Think of a TieredContext as a **flexible, task-oriented bundle** that aggregates relevant *representations*, whether semantic, visual, or raw.

***

## ✅ TieredContext = “LLM-ready or UI-ready View”

> A TieredContext is not limited to IR → JSON views. It’s a composable container that can include:

* Visual representations (like UMLs)
* Extracted chunks (InfoBlocks)
* Literal file slices or full files
* Metadata or annotations
* Inline commentary / diagnostics

***

## 🔁 Example Use Case: "Debugging a Buggy Module"

Let’s say you're building a TieredContext to help a developer (or LLM) debug an issue in the `bank/payment` module.

### 👇 This TieredContext might contain:

### 1. ✅ A generated UML Class Diagram

* Derived from `SymbolIR + TypeIR`
* Rendered as a PNG, SVG, or mermaid string
* Shows object layout and method relations

### 2. ✅ Several InfoBlocks

* `initiate_transfer`, `log_failure`, `send_receipt`
* Marked as "High-Risk" from a static analysis pass
* Each InfoBlock = one self-contained chunk (symbol + scope)

### 3. ✅ Raw File Content (or slices)

* Full text of `payment_controller.py`
* Top 100 lines of `audit_logger.py`

### 4. ✅ Metadata or annotations

* Exception flow notes (`ExceptionIR`)
* Commit metadata / git blame
* "Last touched by XYZ"

***

## 💼 Data Structure Concept

Here’s what a flexible TieredContext object might look like:

```json
{
  "name": "debug_view:payment_issue",
  "context_type": "mixed_debug_bundle",
  "sources": ["SymbolIR", "CallGraphIR", "ExceptionIR", "git"],
  "entities": {
    "uml_diagram": "file://uml/payment_uml.svg",
    "infoblocks": [
      "function:payment.initiate_transfer",
      "function:payment.log_failure",
      "function:payment.send_receipt"
    ],
    "files": [
      { "path": "bank/payment/payment_controller.py", "content": "...source..." },
      { "path": "bank/audit/audit_logger.py", "content": "...slice..." }
    ],
    "metadata": {
      "last_modified": "2025-05-31",
      "blame": ["user_x", "user_y"],
      "exceptions_traced": ["TransferFailedException", "LogWriteError"]
    }
  }
}

```

***

## 🧩 Design Rationale

| Element | Role |
| --- | --- |
| **UML diagram** | High-level semantic map (class/object view) |
| **InfoBlocks** | Granular semantic units (relevant functions/regions) |
| **Files** | Literal context if semantic extraction isn’t enough |
| **Metadata** | Useful overlays: git, errors, timestamps, etc. |

***

## 🎯 Summary

> ✅ Yes, a TieredContext can — and should — include:

* A **UML diagram**
* Some **InfoBlocks**
* Raw **files or file slices**
* Any other contextual scaffolding needed for a task

It’s designed to be the **end product** of parsing, IR, compression, and selection — ready for LLM use, dev tools, or visualization layers.

Would you like a YAML or JSON schema for this type of composite TieredContext, including fields like `uml`, `infoblocks`, `files`, `metadata`, etc.?

You've been admirably rigorous so far — and yes, **we’ve been consistent** in the definitions of the three key layers in your architecture: **IRs**, **InfoBlocks**, and **TieredContexts**. But let’s review them carefully and **audit** for any slippage or edge ambiguity.

***

## ✅ Canonical Definitions (As We’ve Used So Far)

### 1. **IR (Intermediate Representation)**

> A complete, structured semantic model, derived from the AST (and optionally runtime data), focused on a specific aspect of the codebase.

* ❗ Not intended to be shown to humans directly.
* 🧱 Example: `SymbolIR`, `CallGraphIR`, `TypeIR`, `ActivityIR`, `DocIR`
* 📥 Feeds: InfoBlock extractors, TieredContext builders

✅ **Consistency so far**: solid. We’ve never used IR to refer to a partial, sliced, or filtered view — only to complete semantic models.

***

### 2. **InfoBlock**

> A semantically meaningful chunk of code/data, selected for relevance or coherence — it may span multiple IRs, but stays bounded and local.

* ⚡ Purpose: Enables contextual reuse of *units* like "helper group", "main function block", or "class + main method"
* 🎯 Used to assemble TieredContexts or for ad-hoc inclusion
* 📏 Rule of thumb: if it could be a collapsible node in an IDE, it's likely an InfoBlock

✅ **Consistency so far**: yes. We've used it to mean:

* "group of related functions"
* "chunk extracted by a user"
* "self-contained logic unit"

🔍 **Ambiguity to watch**: when an InfoBlock includes visual or non-code elements (e.g., a rendered diagram), that might be better classified as part of a TieredContext **unless** it directly maps to a code unit.

***

### 3. **TieredContext**

> A task-optimized bundle of content — composed from IRs and/or InfoBlocks — to be delivered to downstream consumers (e.g., LLMs, GUI, diagrams).

* 📦 Can include:
  * Rendered diagrams (UML, flowcharts)
  * Code slices (from InfoBlocks or raw files)
  * Metadata (from git, telemetry, etc.)
* 🧠 Structured for **compression**, **injection**, or **visualization**
* 📈 Often scoped by tier level (0 = local file, 1 = module, etc.)

✅ **Consistency so far**: yes. We’ve been treating it as the “final product” meant for:

* Visualizations
* LLM prompt input
* IDE assistance

🔍 **Caution zone**: if a TieredContext contains only one InfoBlock and nothing else, it *might look like* an InfoBlock — but the distinction is in **intent and context**, not size.

***

## 🧮 Summary Table

| Term | What it Encodes | Input From | Output To | Scope |
| --- | --- | --- | --- | --- |
| **IR** |  | AST, runtime | InfoBlock extractor, tier builder | Global/full |
| **InfoBlock** | Coherent unit or chunk (class, helpers, main + logic) | IRs (filtered) | TieredContext, diagram panel | Local/unit |
| **TieredContext** | Bundled view for purpose (LLM, UI, debug) | IRs + InfoBlocks | LLM, GUI, CLI, TUI, diagrams | Mixed/flexible |
