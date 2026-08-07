#!/usr/bin/env python3
"""
CST JSON Analyzer
-----------------
Analyzes a CST JSON file produced by dump_c_cst.py to help diagnose runaway recursion or infinite expansion.

- Prints the most common node types and their counts.
- Computes the maximum depth of the CST.
- Optionally, prints the path to the deepest node.
- Optionally, prints a sample of repeated subtrees.

Usage:
    python3 analyze_cst_json.py <path-to-cst-json>
"""
import sys
import json
from collections import Counter, deque

def walk(node, depth=0, type_counter=None, max_depth_info=None, path=None):
    if type_counter is not None:
        type_counter[node.get('type', '<unknown>')] += 1
    if max_depth_info is not None and depth > max_depth_info[0]:
        max_depth_info[0] = depth
        max_depth_info[1] = list(path)
    children = node.get('children', [])
    for idx, child in enumerate(children):
        path.append((child.get('type', '<unknown>'), idx))
        walk(child, depth + 1, type_counter, max_depth_info, path)
        path.pop()

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 analyze_cst_json.py <path-to-cst-json>", file=sys.stderr)
        sys.exit(1)
    path = sys.argv[1]
    with open(path, 'r', encoding='utf-8') as f:
        root = json.load(f)
    type_counter = Counter()
    max_depth_info = [0, []]  # [max_depth, path_to_deepest]
    walk(root, 0, type_counter, max_depth_info, path=deque())
    print(f"Most common node types:")
    for node_type, count in type_counter.most_common(20):
        print(f"  {node_type}: {count}")
    print(f"\nMaximum CST depth: {max_depth_info[0]}")
    print(f"Path to deepest node: {max_depth_info[1]}")

if __name__ == "__main__":
    main()
