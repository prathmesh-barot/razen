#!/bin/bash
# ── Razen Sample Test Suite ──────────────────────────────────────────────────
# Compiles all samples/*.rzn, links, runs, and reports pass/fail/crash.
# Usage:
#   bash test_Samples.sh              # quick summary
#   bash test_Samples.sh --verbose    # full output per sample
#   bash test_Samples.sh --failed     # only show failures
#   bash test_Samples.sh --list       # list all samples with descriptions

set -e

cd "$(dirname "$0")"

RAZENC="./razenc"
OUTDIR="output"
VERBOSE=false
SHOW_FAILED_ONLY=false
LIST_ONLY=false

for arg in "$@"; do
    case "$arg" in
        --verbose|-v)   VERBOSE=true ;;
        --failed|-f)    SHOW_FAILED_ONLY=true ;;
        --list|-l)      LIST_ONLY=true ;;
    esac
done

if [ ! -f "$RAZENC" ]; then
    echo "Error: $RAZENC not found. Run 'make' first."
    exit 1
fi

mkdir -p "$OUTDIR"

SAMPLES=$(ls samples/0*.rzn 2>/dev/null | sort)
TOTAL=$(echo "$SAMPLES" | wc -l)

if [ "$TOTAL" -eq 0 ]; then
    echo "No .rzn samples found in samples/"
    exit 1
fi

# ── List mode ──────────────────────────────────────────────────────────────
if $LIST_ONLY; then
    echo "Razen Sample Suite — $TOTAL samples"
    echo ""
    for f in $SAMPLES; do
        n=$(basename "$f" .rzn)
        desc=$(head -1 "$f" | sed 's|// ||')
        echo "  $n  $desc"
    done
    exit 0
fi

# ── Categorize what each sample tests (from header comments) ───────────────
get_desc() {
    head -3 "$1" | grep '//' | head -1 | sed 's|// ||'
}

has_ext_func() {
    grep -q 'ext func' "$1"
}

has_use_std() {
    grep -q 'use std\.' "$1"
}

# ── Stats ──────────────────────────────────────────────────────────────────
PASS=0
FAIL_COMPILE=0
FAIL_LINK=0
FAIL_CRASH=0

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║       Razen Sample Test Suite — Compile → Link → Run       ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

for f in $SAMPLES; do
    n=$(basename "$f" .rzn)
    desc=$(get_desc "$f")
    obj="$OUTDIR/$n.o"
    bin="/tmp/rzn_test_$n"

    # ── Compile ──────────────────────────────────────────────────────
    if $VERBOSE; then echo ""; fi
    compile_out=$(./razenc "$f" 2>&1)
    if [ $? -ne 0 ]; then
        FAIL_COMPILE=$((FAIL_COMPILE + 1))
        if $SHOW_FAILED_ONLY || $VERBOSE; then
            echo "  [${n}] FAIL-COMPILE  $desc"
            echo "$compile_out" | sed 's/^/       /'
        fi
        continue
    fi

    # ── Link ──────────────────────────────────────────────────────────
    if [ ! -f "$obj" ]; then
        FAIL_LINK=$((FAIL_LINK + 1))
        if $SHOW_FAILED_ONLY || $VERBOSE; then
            echo "  [${n}] FAIL-LINK (no .o)  $desc"
        fi
        continue
    fi

    link_out=$(gcc -no-pie -o "$bin" "$obj" 2>&1)
    if [ $? -ne 0 ]; then
        FAIL_LINK=$((FAIL_LINK + 1))
        if $SHOW_FAILED_ONLY || $VERBOSE; then
            echo "  [${n}] FAIL-LINK  $desc"
            echo "$link_out" | sed 's/^/       /'
        fi
        continue
    fi

    # ── Run ───────────────────────────────────────────────────────────
    set +e
    run_out=$("$bin" 2>&1)
    rc=$?
    set -e

    # Detect actual signal-caused crash (exit code 128+N where N is 1..31)
    is_signal=false
    if [ $rc -gt 128 ]; then
        sig=$((rc - 128))
        if kill -l "$sig" 2>/dev/null; then
            is_signal=true
        fi
    fi

    if $is_signal; then
        FAIL_CRASH=$((FAIL_CRASH + 1))
        if $SHOW_FAILED_ONLY || $VERBOSE; then
            echo "  [${n}] FAIL-CRASH (signal $sig)  $desc"
            if [ -n "$run_out" ]; then
                echo "$run_out" | sed 's/^/       out> /'
            fi
        fi
        continue
    fi

    PASS=$((PASS + 1))

    # ── Report ──────────────────────────────────────────────────────
    if ! $SHOW_FAILED_ONLY; then
        features=""
        if has_ext_func "$f"; then features="${features}ffi "; fi
        if has_use_std "$f"; then features="${features}std "; fi
        printf "  \033[32m✓\033[0m [%s] exit=%d  %s\n" "$n" "$rc" "$desc"
        if $VERBOSE && [ -n "$run_out" ]; then
            echo "$run_out" | sed 's/^/       out> /'
        fi
    fi
done

# ── Summary ────────────────────────────────────────────────────────────────
echo ""
echo "═══════════════════════════════════════════════════════════════"
printf "  Total:  %3d samples\n" "$TOTAL"
printf "  Pass:   %3d  (compiled + linked + ran without crash)\n" "$PASS"
printf "  Fail:   %3d\n" "$((FAIL_COMPILE + FAIL_LINK + FAIL_CRASH))"
if [ "$FAIL_COMPILE" -gt 0 ]; then printf "    ├─ COMPILE: %d (semantic/parse errors)\n" "$FAIL_COMPILE"; fi
if [ "$FAIL_LINK" -gt 0 ]; then printf "    ├─ LINK:    %d (linker errors)\n" "$FAIL_LINK"; fi
if [ "$FAIL_CRASH" -gt 0 ]; then printf "    └─ CRASH:   %d (runtime crash/signal)\n" "$FAIL_CRASH"; fi
echo "═══════════════════════════════════════════════════════════════"

exit $((FAIL_COMPILE + FAIL_LINK + FAIL_CRASH))
