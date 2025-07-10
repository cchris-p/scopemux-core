#!/bin/bash

EXCLUDE_PATTERNS=("vendor" "build-*" "_deps")

find_cmd=(find .)

for pattern in "${EXCLUDE_PATTERNS[@]}"; do
  find_cmd+=(-path "./$pattern" -prune -o)
done

find_cmd+=(-type f -name "CMakeLists.txt" -print0)

"${find_cmd[@]}" | while IFS= read -r -d '' file; do
  echo "==> $file"
  cat "$file"
done
