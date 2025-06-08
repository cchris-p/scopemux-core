# ScopeMux Project Definitions

Project Indexing Stages

* Raw Parsed Artifacts
* InfoBlocks
* Tiered Context
* Semantics
* Documentation

## InfoBlocks

InfoBlocks are the fundamental, atomic semantic units of contextual information used to construct relevant prompts or context for an LLM agent. An InfoBlock represents any logically extractable, encapsulated chunk of code or documentation—essentially, anything that can be selected as a meaningful region within a file or project. InfoBlocks can overlap and nest, forming a tree or DAG structure (e.g., a method inside a class, inside a file). This hierarchy is used for assembling higher-level context bundles (Tiered Contexts).

Think of InfoBlocks as Lego bricks that can be assembled into more complex structures.

**Examples of Info Blocks:**

* Function definition (with name, signature, and docstring)
* Class or struct definition (including methods, members, and class-level docstrings)
* Individual method or constructor within a class
* Module-level docstring or comment block
* Enum or typedef declaration
* File-level license or copyright header
* Global constant or macro definition
* Conditional block (`if`, `elif`, `else`, `switch` statements)
* Loop block (`for`, `while`, etc.)
* Import/include statement
* Top-level variable or assignment
* Build script (e.g., `CMakeLists.txt` or `Makefile` snippet)
* README or documentation section
* Configuration block (e.g., YAML, JSON, TOML snippet)
* Decorator or annotation block (Python, Java, etc.)
* Test case or test function
* API endpoint handler
* UI component definition
* Inline comment with specific context (e.g., TODO/FIXME with code region)
* Directory-level summary (e.g., an `__init__.py` or index file)

> InfoBlocks are intentionally lightweight, reusable, and overlap naturally: the body of an `if` statement is an InfoBlock, but so is the containing function, the containing class, and the file itself.

## Tiered Contexts

Tiered Contexts are user-defined or automated views or assemblies of InfoBlocks, filtered or grouped by abstraction level (Tier 0–3), designed to provide just enough relevant context for a given query or coding task. They are lightweight, encodable, and can be composed based on code additions, features, or user requests. Tiered Contexts allow for rapid reference and context expansion in chat or search tools, while staying within token budgets.

These are to be defined by a user through a config. Tiered Contexts can be easily pulled/referenced in the chat feature. They represent a structured context configuration that can be applied either globally or within a composition of InfoBlocks.

An example of a Tiered Context formation:

```

All Auth Utility Functions

Auth Backend Endpoints and Views

All Auth UI Components

```

In this example as well as in every situation, selected Tiered Contexts are fundamentally made up of many compositions of InfoBlocks, very much like a depth/breadth traversal in a graph of many levels. While Tiered Contexts can represent compositions of InfoBlocks, not all compositions need to become Tiered Contexts.

Since InfoBlocks are lightweight packets of information, we aren't required to include every deep-rooted node because that may complicate the query and defeat the purpose of Tiered Contexts. It may be in our best interest to include a max level of context we'd want to be included in the Tiered Context. This is controlled through the tier levels (0-3), which filter how much of each block is shown. Future plans to automate this behavior are considered in this project.

**Examples Tiered Context definitions:**

```
All Database Models and their Relationships

Critical Bug Fix Commits (last 30 days)

README + Top-level Build Instructions

All Test Cases for Payment Processing

Config Defaults + Environment Variable Definitions

Docker-related Files
```

Tiered Contexts are constructed as traversals or compositions of InfoBlocks—either breadth/depth-first, or by semantic grouping. They may be hierarchical (e.g., all methods in a class), but can also be cross-cutting (e.g., all login-related code in multiple files). The context depth, breadth, or specificity can be controlled through tier levels (e.g., showing a composition at Tier 1 only). Tiered Contexts can be auto-generated (e.g., all Tier 1 classes in `/models`) or user-defined (e.g., business logic Tier 2 for checkout flow).

## Compositions

Compositions are handpicked or algorithmically selected sets of InfoBlocks (possibly across tiers) meant to fulfill a specific purpose. They represent custom semantic bundles of blocks based on purpose (e.g., "everything related to onboarding" or "core auth flow").

Key characteristics of compositions:
* They can be treated as proto-Tiered Contexts, but don't have to obey tier boundaries
* They may cross files, directories, or even languages (e.g., one Python validator + one TypeScript frontend hook)
* They are task-driven bundles that can optionally be filtered through a Tiered Context lens

## Semantics

Semantics are formalized representations or models of code structure, behavior, or relationships. These are typically generated from InfoBlocks and Tiered Contexts, and are used to visualize or summarize key aspects of the codebase.

The project's modules that are eligible to be built out as diagrams or informational representations via Semantics must to be defined by the user.

**Examples of Semantic Models:**

* `Call Graph`: Relationships and dependencies between functions/methods
* `Activity Diagram`: Execution paths, control flow, or sequence diagrams (UML, trace)
* `Class Inheritance Tree`: Visualization of OO class hierarchies
* `Module Import Graph`: Dependency relationships between files/modules/packages
* `Reference Map`: Locations where symbols (functions, classes, variables) are used
* `Test Coverage Matrix`: Mapping of test cases to covered functions/modules

> Only modules or files explicitly selected by the user are eligible for semantic modeling. This keeps diagrams and summaries focused and relevant.

# Distinguishing Between Tiered Contexts and InfoBlocks

With the core concepts defined above, it's important to clarify how these components interact within the system architecture. This section provides further guidance on the relationships between these concepts.

### Key Insight

> A composition of InfoBlocks can be expressed as a Tiered Context — but not all Tiered Contexts are built this way, and not all InfoBlock compositions need to become Tiered Contexts.

***

## 🧩 Relationships

### ✅ A Composition of InfoBlocks is:

* A **custom semantic bundle** of blocks based on purpose (e.g., “everything related to onboarding”, or “core auth flow”).
* Can be treated as a **proto–Tiered Context**, but doesn’t *have to* obey tier boundaries.
* May cross files, directories, or even languages (e.g., one Python validator + one TypeScript frontend hook).

***

### ✅ A Tiered Context is:

* A **structured context configuration** (Tier 0–3) applied **either globally or within a composition**.
* Could be applied *after* building a composition (e.g., “show me this composition at Tier 1 only”).
* Can be auto-generated (e.g., “all Tier 1 classes in `/models`”) or user-defined (e.g., “business logic Tier 2 for checkout flow”).

***

## Relationship Classification

Addressing the distinction between these concepts:

* **Compositions of InfoBlocks** ✅

  Hand-selected, filtered, or agent-resolved semantic bundles.

* **Optionally filtered through a Tiered Context lens** ✅

  You can request: "Give me this composition at Tier 2" — this filters how much of each block is shown.

* **Not necessarily full Tiered Contexts themselves** ❌

  Unless you define them that way. Tiered Contexts tend to have **structure + rules**, while compositions are **task-driven bundles**.

***

## Practical Implementation

| Concept | Example | Source | Format |
| --- | --- | --- | --- |
| **InfoBlock** | `send_email()` | Parsed from IR | Flat |
| **Composition** | `send_email()` + `validate()` + `EmailValidator` | User/Agent curated | Flat/semantic |
| **Tiered Context** | “All Tier 1 InfoBlocks in `auth/`” | Auto-filtered | Tiered |
| **Composition + Tiered Context** | “Auth flow classes at Tier 2 only” | Filtered + sliced | Structured (hierarchical) |

***

## Implementation Approach: Compositions as Templates

> You can treat any InfoBlock composition as a named Tiered Context template — e.g.:

```json
{  "name": "onboarding_email_flow",  "blocks": ["EmailSender", "validate_input", "send_email"],  "tier_filter": 1,  "masking": ["function_bodies"],  "purpose": "refactor and add async support"}
```

***

A YAML or JSON schema could be defined for this hybrid object (`composition + tiered context + masking`) to drive LLM task inputs.
