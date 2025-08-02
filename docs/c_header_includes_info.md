Absolutely! Let’s break down the **three main types of includes** in C, how they appear in code, and how your `includes.scm` query file is designed to match them.

---

## **Types of C Includes**

### 1. **System Header Include**
- **Syntax:**  
  ```c
  #include <stdio.h>
  ```
- **Purpose:**  
  Includes standard library or system headers.
- **Brackets:**  
  Uses angle brackets `<...>`.
- **Tree-sitter Node:**  
  Usually represented as a `preproc_include` node with a child node like `system_lib_string`.

---

### 2. **System Include with Path**
- **Syntax:**  
  ```c
  #include <dir/myheader.h>
  ```
- **Purpose:**  
  Includes a header from a system or library path, possibly with subdirectories.
- **Brackets:**  
  Also uses angle brackets `<...>`.
- **Tree-sitter Node:**  
  `preproc_include` with a `path` field containing a `system_lib_string`.

---

### 3. **Local Include with Path**
- **Syntax:**  
  ```c
  #include "myheader.h"
  #include "dir/other.h"
  ```
- **Purpose:**  
  Includes a header from the local project directory or a relative path.
- **Brackets:**  
  Uses double quotes `"..."`.
- **Tree-sitter Node:**  
  `preproc_include` with a `path` field containing a `string_literal`.

---

## **How Your `includes.scm` Handles These**

### **1. System Header Include**
```scheme
(preproc_include
  (system_lib_string) @system_header_path) @system_include
```
- **Matches:**  
  `#include <stdio.h>`
- **Captures:**  
  - `@system_header_path`: the header name (e.g., `<stdio.h>`)
  - `@system_include`: the whole include node
- **Use:**  
  For standard library/system headers without a `path:` field.

---

### **2. System Include with Path**
```scheme
(preproc_include
  path: (system_lib_string) @name) @include
```
- **Matches:**  
  `#include <dir/myheader.h>`
- **Captures:**  
  - `@name`: the path (e.g., `<dir/myheader.h>`)
  - `@include`: the whole include node
- **Use:**  
  For system includes that have a `path:` field (tree-sitter sometimes parses includes with a `path:` field).

---

### **3. Local Include with Path**
```scheme
(preproc_include
  path: (string_literal) @name) @include
```
- **Matches:**  
  `#include "myheader.h"`
  `#include "dir/other.h"`
- **Captures:**  
  - `@name`: the path (e.g., `"myheader.h"`)
  - `@include`: the whole include node
- **Use:**  
  For local includes with double quotes.

---

### **4. Generic Fallback**
```scheme
(preproc_include) @include
```
- **Matches:**  
  Any `preproc_include` not matched above.
- **Captures:**  
  - `@include`: the whole include node
- **Use:**  
  As a last resort, should rarely be used if the above patterns are correct.

---

## **Summary Table**

| C Code Example                | Pattern in `.scm`                | Capture(s)         | Node Name Should Be      |
|-------------------------------|----------------------------------|--------------------|--------------------------|
| `#include <stdio.h>`          | Lines 2-3                        | `@system_header_path`, `@system_include` | `<stdio.h>`              |
| `#include <dir/myheader.h>`   | Lines 6-7                        | `@name`, `@include`| `<dir/myheader.h>`       |
| `#include "myheader.h"`       | Lines 10-11                      | `@name`, `@include`| `"myheader.h"`           |
| `#include "dir/other.h"`      | Lines 10-11                      | `@name`, `@include`| `"dir/other.h"`          |
| Anything else                 | Lines 14                         | `@include`         | fallback (should be rare)|

---

## **How It Works in Practice**

- **Tree-sitter** parses the C code and produces a syntax tree.
- **Your `.scm` queries** match specific patterns in that tree:
  - If the include is a system header (no `path:` field), the first pattern matches.
  - If the include has a `path:` field and uses angle brackets, the second pattern matches.
  - If the include has a `path:` field and uses quotes, the third pattern matches.
  - If none of the above, the fallback matches.
- **Your parser** should use the most specific capture available to name the node.

---

## **Key Point**
- **System includes** use angle brackets and may or may not have a `path:` field depending on the parser/tree-sitter version.
- **Local includes** use quotes and always have a `path:` field with a `string_literal`.
- **Fallback** should only be used if the above do not match.

---

If you want, I can show you how to adjust your parser logic to always prefer the most specific match and avoid unnamed/fallback nodes. Let me know!