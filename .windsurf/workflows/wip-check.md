---
description: Work-In-Progress (WIP) Check
---

**Purpose**  
Quickly answer “any luck? where are we?” *before committing* by inspecting **both** staged and unstaged edits, then summarising:

* **Main task in focus** (the feature/bug‐fix you’re working on)  
* **Related subtasks** (helpers, refactors, docs, etc., that move the main task forward)  
* **Unrelated tasks** (drive‐by fixes or tangents)  
* % done vs. % left

---

### 0 - Inputs
* **`$ASK`** – the natural-language question from the teammate (passed automatically by Cascade).  
* Optional **`--scope <dir>`** flag to narrow the review.

---

### 1 - Detect the *main task*
1. Look for a JIRA/issue key, Trello card ID, or GH issue number inside **`$ASK`**.  
2. Else, pull the identifier from the current branch name (`git rev-parse --abbrev-ref HEAD`).  
3. Else, prompt the user: “What’s the primary task ID / description right now?”  

Store as **`$MAIN`**.

---

### 2 - Take two snapshots
```bash
# Staged (index) delta
git diff --cached --name-status > /tmp/wip_staged.txt
git diff --cached                > /tmp/wip_staged.patch

# Unstaged (working tree) delta
git diff --name-status           > /tmp/wip_unstaged.txt
git diff                         > /tmp/wip_unstaged.patch
````

---

### 3 - Mine progress markers

```bash
grep -RIn \
  -e "TODO" -e "FIXME" -e "@in-progress" \
  --exclude-dir=.git ${SCOPE:-.} > /tmp/wip_tags.txt
```

*Tag lines present in **staged or unstaged** snapshots are *pending*.*
*Removed tags inside patches are *done*.*

---

### 4 - Classify tasks

* **Related** ↔ file path or tag contains `$MAIN`.
* **Unrelated** ↔ everything else.

For each patch chunk:

1. If it adds significant code (>5 non-blank lines) *and* touches `$MAIN` files → mark **done-main**.
2. If it removes or resolves a marker in those files → **done-main**.
3. Same logic for non-main files → **done-related** or **done-unrelated**.

Count:

```
done_main, done_related, done_unrelated
pending_main, pending_related, pending_unrelated
total = done_* + pending_*
```

---

### 5 - Compute percentages

```
complete_pct = floor((done_main + done_related) / (total_main + total_related) * 100)
remain_pct   = 100 - complete_pct
```

*Unrelated work is reported but **excluded** from headline % so it doesn’t dilute focus.*

---

### 6 - Compose the reply

```
Progress Review: $MAIN

Main Task & Direct Subtasks (<complete_pct>% Complete)
✅ <summary 1 from done_main/done_related>
✅ <summary 2 …>

Remaining (<remain_pct>% Left)
❌ <pending_main item 1>
❌ <pending_related item 1>

Tangential / Opportunistic Work (not counted in %)
• ✅ / ❌ <done_unrelated or pending_unrelated>

Next Steps
• Highest-impact pending_main
• …
```

---

### 7 - Offer actions

* “Commit staged as ‘WIP: \$MAIN snapshot’?”
* “Stage unstaged edits?”
* “Drill into `<path>` for finer metrics?”

---

### 8 - File placement

Save this as `.windsurf/workflows/wip-review.md`.
Invoke with `/wip-review [--scope src/module]` whenever you want a checkpoint report that considers **everything in the index *and* the working directory**.

