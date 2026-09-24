# Durable Plan Store

ScopeMux separates its storage into two layers with a hard ownership boundary.

## Derived store (disposable)

Everything produced by parsing is **derived** and safe to discard:

- CST/AST parse results
- `ProjectIRSnapshot` (Symbol IR, resolved references, call graph, dependencies)
- `ProjectInfoBlockRegistry` (canonical InfoBlocks)
- search index and related-block links

`project_context_rebuild_ir()` / `project_context_rebuild_info_blocks()` fully
regenerate these from source. They are content-addressable by construction and
carry no state that cannot be recomputed.

## Durable store (plan state)

Target-state plan nodes are **durable** and must survive re-index and restart:

- `ProjectPlanNode` records: stable id, kind, title, desired shape, rationale
- lifecycle, provenance, confidence
- anchors into current-state blocks
- projected symbol / file path

The durable store lives on `ProjectContext` (`plan_nodes`, `plan_node_count`)
and is owned by the plan-node API in `tiered_context.c`. A derived re-index
re-projects plan nodes into the registry but never regenerates or deletes them
(`project_context_clear_ir` does not touch the plan store; only project
destruction or `project_context_clear_plan_nodes` does).

## Serialization

`plan_store.c` serializes the durable store to a stable, machine-readable JSON
schema (`version` + `plan_nodes[]`). JSON is chosen for portability and
debuggability; a binary format can be added later without changing the
load/save contract.

### API

| Function | Purpose |
| --- | --- |
| `project_context_plan_nodes_to_json` | serialize the durable store |
| `project_context_plan_nodes_from_json` | load (replace or merge by id) |
| `project_context_plan_nodes_save` | write the durable store to a file |
| `project_context_plan_nodes_load` | read the durable store from a file |

`merge == false` replaces the plan store; `merge == true` adds or updates nodes
by id and preserves unrelated nodes.

## Guarantees

- Restart with `load` preserves all plan nodes and map metadata.
- Deleting the derived cache does not lose plan state.
- Re-index reconciles (`project_context_reconcile_plan_nodes`) and marks
  affected nodes `stale`/`conflict` rather than dropping them.
- The external task record remains authoritative; the store never advances a
  task stage or completion.
