#!/bin/bash
# Generate one dump file per MAIN directory (recursive, all subdirs included)

BASE="/home/user/Phoenix01/rael_v48_clean"
OUT="/home/user/Phoenix01/code_dumps"

dump_recursive() {
    local dir="$1"
    local outfile="$2"
    > "$outfile"
    local count=0

    while IFS= read -r -d '' f; do
        # Skip binary files by extension
        case "$f" in
            *.exe|*.so|*.dll|*.o|*.a|*.png|*.jpg|*.jpeg|*.gif|*.bmp|*.ico|*.pdf) continue ;;
        esac
        # Skip files without extension that are binaries
        local fname=$(basename "$f")
        if [[ "$fname" != *.* ]]; then
            file "$f" | grep -q "text\|empty\|JSON\|XML\|ASCII" || continue
        fi

        local relpath="${f#$BASE/}"
        echo "### ${relpath} ###" >> "$outfile"
        cat "$f" >> "$outfile"
        echo "" >> "$outfile"
        echo "### eof ###" >> "$outfile"
        echo "" >> "$outfile"
        count=$((count + 1))
    done < <(find "$dir" -type f -print0 | sort -z)

    echo "  -> $(basename "$outfile") ($count files, $(du -h "$outfile" | cut -f1))"
}

echo "=== Generating main directory dumps ==="

# Root-level files only (no subdirs)
echo "Processing: root"
{
    outfile="$OUT/root.txt"
    > "$outfile"
    count=0
    for f in "$BASE"/*; do
        [ -f "$f" ] || continue
        case "$f" in *.exe|*.so|*.dll|*.o|*.a|*.png) continue ;; esac
        fname=$(basename "$f")
        if [[ "$fname" != *.* ]]; then
            file "$f" | grep -q "text\|empty\|JSON\|XML\|ASCII" || continue
        fi
        relpath="${f#$BASE/}"
        echo "### ${relpath} ###" >> "$outfile"
        cat "$f" >> "$outfile"
        echo "" >> "$outfile"
        echo "### eof ###" >> "$outfile"
        echo "" >> "$outfile"
        count=$((count + 1))
    done
    echo "  -> root.txt ($count files, $(du -h "$outfile" | cut -f1))"
}

echo "Processing: build"
dump_recursive "$BASE/build" "$OUT/build.txt"

echo "Processing: config"
dump_recursive "$BASE/config" "$OUT/config.txt"

echo "Processing: cuda"
dump_recursive "$BASE/cuda" "$OUT/cuda.txt"

echo "Processing: docs"
dump_recursive "$BASE/docs" "$OUT/docs.txt"

echo "Processing: example_pack"
dump_recursive "$BASE/example_pack" "$OUT/example_pack.txt"

echo "Processing: include (all)"
dump_recursive "$BASE/include" "$OUT/include.txt"

echo "Processing: modules (all)"
dump_recursive "$BASE/modules" "$OUT/modules.txt"

echo "Processing: python"
dump_recursive "$BASE/python" "$OUT/python.txt"

echo "Processing: src (all)"
dump_recursive "$BASE/src" "$OUT/src.txt"

echo "Processing: tests"
dump_recursive "$BASE/tests" "$OUT/tests.txt"

echo "Processing: .rael"
dump_recursive "$BASE/.rael" "$OUT/dot_rael.txt"

echo ""
echo "=== Done ==="
