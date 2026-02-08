#!/bin/bash
# Script to generate code dump files per directory
# Format: ### directory/filename ### \n code \n ### eof ###

BASE="/home/user/Phoenix01/rael_v48_clean"
OUT="$BASE/code_dumps"

# Function: dump all text files in a directory (non-recursive) into one output file
dump_dir() {
    local dir="$1"       # absolute path to source dir
    local outname="$2"   # output filename (without .txt)
    local outfile="$OUT/${outname}.txt"

    # Clear/create output file
    > "$outfile"

    local count=0
    for f in "$dir"/*; do
        [ -f "$f" ] || continue

        # Skip binary files
        case "$f" in
            *.exe|*.so|*.dll|*.o|*.a|*.png|*.jpg|*.jpeg|*.gif|*.bmp|*.ico|*.pdf)
                continue ;;
        esac

        # Skip files without extension that are likely binaries (check with file command)
        local fname=$(basename "$f")
        if [[ "$fname" != *.* ]]; then
            # Check if it's a text file
            if file "$f" | grep -q "text\|empty\|JSON\|XML\|ASCII"; then
                : # it's text, continue
            else
                continue  # skip binary
            fi
        fi

        # Get relative path from rael_v48_clean
        local relpath="${f#$BASE/}"

        echo "### ${relpath} ###" >> "$outfile"
        cat "$f" >> "$outfile"
        echo "" >> "$outfile"
        echo "### eof ###" >> "$outfile"
        echo "" >> "$outfile"
        count=$((count + 1))
    done

    if [ $count -eq 0 ]; then
        rm -f "$outfile"
        echo "  (no text files, skipped)"
    else
        echo "  -> $outfile ($count files)"
    fi
}

echo "=== Generating code dumps ==="

# 1. Root directory
echo "Processing: root"
dump_dir "$BASE" "00_root"

# 2. .rael
echo "Processing: .rael"
dump_dir "$BASE/.rael" "01_dot_rael"

# 3. build
echo "Processing: build"
dump_dir "$BASE/build" "02_build"

# 4. config
echo "Processing: config"
dump_dir "$BASE/config" "03_config"

# 5. cuda
echo "Processing: cuda"
dump_dir "$BASE/cuda" "04_cuda"

# 6. docs (skip images subdir)
echo "Processing: docs"
dump_dir "$BASE/docs" "05_docs"

# 7. example_pack (including payload subdir)
echo "Processing: example_pack"
{
    > "$OUT/06_example_pack.txt"
    for f in "$BASE/example_pack"/* "$BASE/example_pack/payload"/*; do
        [ -f "$f" ] || continue
        relpath="${f#$BASE/}"
        echo "### ${relpath} ###" >> "$OUT/06_example_pack.txt"
        cat "$f" >> "$OUT/06_example_pack.txt"
        echo "" >> "$OUT/06_example_pack.txt"
        echo "### eof ###" >> "$OUT/06_example_pack.txt"
        echo "" >> "$OUT/06_example_pack.txt"
    done
    echo "  -> $OUT/06_example_pack.txt"
}

# 8. include/rael (main headers)
echo "Processing: include/rael"
dump_dir "$BASE/include/rael" "07_include_rael"

# 9. include/rael/v49
echo "Processing: include/rael/v49"
dump_dir "$BASE/include/rael/v49" "08_include_rael_v49"

# 10. modules
echo "Processing: modules/sample_math_formulas"
dump_dir "$BASE/modules/sample_math_formulas" "09_modules_math"

echo "Processing: modules/sample_semantic_quint"
dump_dir "$BASE/modules/sample_semantic_quint" "10_modules_semantic"

# 11. python
echo "Processing: python"
dump_dir "$BASE/python" "11_python"

# 12. src root-level files
echo "Processing: src (root)"
dump_dir "$BASE/src" "12_src_root"

# 13. src/cli
echo "Processing: src/cli"
dump_dir "$BASE/src/cli" "13_src_cli"

# 14. src/core
echo "Processing: src/core"
dump_dir "$BASE/src/core" "14_src_core"

# 15. src/cuda
echo "Processing: src/cuda"
dump_dir "$BASE/src/cuda" "15_src_cuda"

# 16. src/gui
echo "Processing: src/gui"
dump_dir "$BASE/src/gui" "16_src_gui"

# 17. src/security
echo "Processing: src/security"
dump_dir "$BASE/src/security" "17_src_security"

# 18. src/tui
echo "Processing: src/tui"
dump_dir "$BASE/src/tui" "18_src_tui"

# 19. src/v50
echo "Processing: src/v50"
dump_dir "$BASE/src/v50" "19_src_v50"

# 20. src/webgui
echo "Processing: src/webgui"
dump_dir "$BASE/src/webgui" "20_src_webgui"

# 21. src/windows
echo "Processing: src/windows"
dump_dir "$BASE/src/windows" "21_src_windows"

# 22. tests
echo "Processing: tests"
dump_dir "$BASE/tests" "22_tests"

echo ""
echo "=== Done! ==="
echo "Files created in: $OUT"
ls -lh "$OUT"/*.txt 2>/dev/null
