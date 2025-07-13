---
description: Refocus the assistant on the primary task after extensive iteration with a “thinking” model. Generates a structured report that (a) recaps completed fixes, (b) rates current hypotheses with confidence percentages, (c) enumerates alternative hypothese
---

## Steps

1. **Ensure primary task is defined**
   • If `PRIMARY_TASK` has already been stored in context, retrieve it.
   • Otherwise, ask the user:

   ```
   What is the primary task you want to remain focused on?
   ```

   • Store the answer as `PRIMARY_TASK` for this workflow run.

2. **Generate Progress Summary**
   • Review the conversation so far.
   • Produce a bullet‑point list of *completed fixes / actions* relevant to `PRIMARY_TASK`.
   • Highlight timestamps or message IDs for traceability.

3. **Detect Divergence**
   • Identify conversation segments that did **not** directly advance `PRIMARY_TASK`.
   • For each divergence, record: the tangent topic, why the model pursued it, whether it yielded value, and whether follow‑up is required.

4. **Confidence Assessment**
   • State the *current best hypothesis* for the root cause or solution.
   • Assign a numeric confidence percentage (`0–100`).
   • List up to 3 alternate hypotheses with their own confidence percentages.
   • Base numbers on evidence collected; do **not** invent precision.

5. **Unexplored & Deferred Paths**
   • List meaningful avenues that were *skipped*, *parked*, or *not yet explored*.
   • For each, give a one‑sentence rationale for why it may still matter.

6. **Next‑Step Plan**
   • Propose a short, ordered checklist (≤5 items) to advance `PRIMARY_TASK`.
   • Mark any steps that **require user input**.

7. **Present Consolidated Report**
   • Output a markdown section with these headers:

   ```
   ## Progress
   ## Divergence
   ## Confidence
   ## Unexplored Paths
   ## Next Steps
   ```

   • Keep the tone concise and factual.

8. **Offer to Branch or Continue**
   • Ask the user whether to:

   1. Continue with the next step,
   2. Explore one of the unexplored paths, or
   3. Re‑evaluate the hypotheses.
