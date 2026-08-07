#!/bin/bash -e

THIS_DIR=$(dirname -- "$(readlink -f -- "${BASH_SOURCE[0]}")")
README="$THIS_DIR/README.md"
BIN=$(mktemp)
trap 'rm -f "$BIN"' EXIT

echo "# Benchmark" >"$README"

for entry in \
  "Bump allocator:bump_allocator" \
  "Implicit free list:implicit_free_list" \
  "Implicit free list IMPROVED:implicit_free_list-improved" \
  "Implicit free list + coalesce:implicit_free_list_coalasce" \
  "Explicit free list:list_explicit_free"; do
  name="${entry%%:*}"
  dir="${entry##*:}"

  echo "Benchmarking $name..."
  gcc -O2 -DALLOC_HEADER="\"$THIS_DIR/../$dir/allocator.h\"" \
    "$THIS_DIR/benchmark.c" "$THIS_DIR/../$dir/allocator.c" -o "$BIN"

  {
    echo ""
    echo "## $name"
    echo ""
    "$BIN"
  } >>"$README"
done

echo "Done. Results in $README"
