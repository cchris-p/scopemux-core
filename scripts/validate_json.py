#!/usr/bin/env python3
import sys
import json

if len(sys.argv) < 2:
    print("Usage: validate_json.py <file1.json> [<file2.json> ...]")
    sys.exit(2)

all_valid = True
for filename in sys.argv[1:]:
    try:
        with open(filename, 'r') as f:
            json.load(f)
        print(f"VALID: {filename}")
    except Exception as e:
        print(f"INVALID: {filename}\n  Error: {e}")
        all_valid = False

sys.exit(0 if all_valid else 1) 