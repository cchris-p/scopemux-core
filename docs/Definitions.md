# ScopeMux - Project Definitions

Project Indexing Stages

* Raw Parsed Artifacts
* InfoBlocks
* Tiered Context
* Semantics
* Documentation

## Info Blocks

Info Blocks are the fundamental, composable units of contextual information used to construct relevant prompts or context for an LLM agent. An Info Block represents any logically extractable, encapsulated chunk of code or documentation—essentially, anything that can be selected as a meaningful region within a file or project. Info Blocks can overlap and nest, forming a tree or DAG structure (e.g., a method inside a class, inside a file). This hierarchy is used for assembling higher-level context bundles (Tiered Contexts).

Info Blocks are code snippets that are utilized as building blocks to consolidate relevant information related to the task presented to the LLM agent. This is the fundamental unit that will be used to build Tiered Conexts to be submitted in the chat. Examples of what info block can be include, but are not limited to, functions, classes, modules, files, conditionals, directories, etc. It is best to imagine that any useful information that can be defined as a selected section of a file (wrapped by what context it serves) can be defined as a type of info block. Some examples of what I'm referring to include docstrings, inline documentation, readme's like this one, build scripts, literally anything interpretable and encapsulate-able. An important note is that there is much overlap with Info blocks in general. For example, all the code within the info block of an if conditional exists within an info block of the function and also exists within the info block of a class. There is a tree-like mapping between these parent-child relationship, which is a critical utility to how Tiered Contexts are formed.

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

> Info Blocks are intentionally lightweight, reusable, and overlap naturally: the body of an `if` statement is an Info Block, but so is the containing function, the containing class, and the file itself.

## Tiered Contexts

Tiered Contexts are user-defined or automated groupings of Info Blocks, designed to provide just enough relevant context for a given query or coding task. They are lightweight, encodable, and can be composed based on code additions, features, or user requests. Tiered Contexts allow for rapid reference and context expansion in chat or search tools, while staying within token budgets.

These are to be defined by a user through a config. Tiered contexts can be easily pulled/referenced in the chat feature. They are meant to be lightweight and encodable snippets that are built based upon any code addition or feature.

An example of a Tiered Context formation:

```

All Auth Utility Functions

Auth Backend Endpoints and Views

All Auth UI Components

```

In this example as well as in every situation, selected Tiered Contexts are fundamentally made up of many compositions of InfoBlocks, very much like a depth/breath traversal in a graph of many levels. But it would also seem that the example is a particularly higher form of Tiered Context. Since Info Blocks are lightweight packets of information, we aren't required to include every deep-rooted node because that may complicate the query and defeat the purpose of Tiered Contexts. It may be in our best interest to include a max level of context we'd want to be included in the Tiered Context. That is an attibute of any Info Block that is pulled into being included in a TieredContext built out by the user's request. Future plans to automate this behavior are considered in this project.

**Examples Tiered Context definitions:**

```
All Auth Utility Functions

Auth Backend Endpoints and Views

All Auth UI Components

All Database Models and their Relationships

Critical Bug Fix Commits (last 30 days)

README + Top-level Build Instructions

All Test Cases for Payment Processing

Config Defaults + Environment Variable Definitions
```

Tiered Contexts are constructed as traversals or compositions of Info Blocks—either breadth/depth-first, or by semantic grouping. They may be hierarchical (e.g., all methods in a class), but can also be cross-cutting (e.g., all login-related code in multiple files). The context depth, breadth, or specificity can be controlled (e.g., limit to 2 levels deep, or include only top-level functions).

## Semantics

Semantics are formalized representations or models of code structure, behavior, or relationships. These are typically generated from Info Blocks and Tiered Contexts, and are used to visualize or summarize key aspects of the codebase.

The project's modules that are eligible to be built out as diagrams or informational representations via Semantics must to be defined by the user.

**Examples of Semantic Models:**

* `Call Graph`: Relationships and dependencies between functions/methods
* `Activity Diagram`: Execution paths, control flow, or sequence diagrams (UML, trace)
* `Class Inheritance Tree`: Visualization of OO class hierarchies
* `Module Import Graph`: Dependency relationships between files/modules/packages
* `Reference Map`: Locations where symbols (functions, classes, variables) are used
* `Test Coverage Matrix`: Mapping of test cases to covered functions/modules

> Only modules or files explicitly selected by the user are eligible for semantic modeling. This keeps diagrams and summaries focused and relevant.
