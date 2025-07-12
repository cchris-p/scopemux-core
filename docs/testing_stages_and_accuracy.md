# ScopeMux Parser Testing Stages: Semantic vs. Full-Fidelity

## Overview
ScopeMux uses a multi-stage testing approach to validate the correctness of its parser output. Understanding these stages is critical for interpreting test results and ensuring parser accuracy.

---

## 1. Semantic AST Extraction Tests (Query-Driven)

- **Purpose:** Check that the parser can extract high-level semantic constructs (e.g., functions, variables, classes) using Tree-sitter queries.
- **Mechanism:**
  - The parser runs Tree-sitter queries for each construct.
  - The extracted AST nodes are compared to `.expected.json` golden files for each test case.
- **Pass Criteria:**
  - The test passes if the extracted nodes match the expected JSON **for the constructs that were actually extracted and compared**.
  - If a query fails (e.g., `variables` query returns no nodes), that part is not checked; the test may still pass if no mismatch is detected for the other constructs.
- **Caveat:**
  - A passing test does **not** guarantee a 1:1 match for all constructs. It only guarantees correctness for the constructs that were successfully queried and compared.

### Example
If the `functions` query works and matches, but `variables` fails (no nodes extracted), the test passes as long as the JSON for `variables` is empty or not checked.

---

## 2. Full-Fidelity (Golden/Integration) Tests

- **Purpose:** Ensure the parser output is a complete, 1:1 match with the golden files for **all** constructs and structure.
- **Mechanism:**
  - The entire parser output (AST or CST) is compared to a golden file.
  - Tests fail if any part of the output is missing, extra, or mismatched.
  - May include schema validation and round-trip (CST) checks.
- **Pass Criteria:**
  - The test passes **only** if the parser output matches the golden file exactly, for all constructs and structure.
- **Guarantee:**
  - This stage provides the strictest assurance of parser correctness.

---

## Summary Table

| Test Passes? | All Queries Succeed? | All Constructs Matched? | 1:1 Match Guarantee? |
|--------------|----------------------|------------------------|----------------------|
| Yes          | No                   | No                     | No                   |
| Yes          | Yes                  | Yes                    | Yes                  |

---

## Recommendations
- Use semantic (query-driven) tests for fast feedback and incremental development.
- Use full-fidelity (golden) tests for strict parser validation before releases or refactoring.
- Investigate and fix any failing queries to avoid missing constructs in semantic tests.
- For strict 1:1 matching, ensure all queries succeed and all constructs are present in the golden files.

---

## Further Reading
- See the project documentation on [AST vs. CST](./ast_vs_cst.md) for more details on tree types.
- Review the test harness code and `.expected.json` files for specifics on what is compared in each test.

---

*Document generated automatically to clarify parser testing stages and accuracy in ScopeMux.*
