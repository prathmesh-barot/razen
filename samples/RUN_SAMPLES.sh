#!/bin/bash
# Compile all 50 samples and report results
# Usage: bash samples/RUN_SAMPLES.sh [--verbose]

set -e

RAZENC="./razenc"
OUTPUT_DIR="output"
VERBOSE=""
PASS=0
FAIL=0

if [ ! -f "$RAZENC" ]; then
    echo "Error: razenc not found. Run 'make' first."
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

if [ "$1" == "--verbose" ] || [ "$1" == "-v" ]; then
    VERBOSE="-v"
fi

echo "=== Razen Sample Test Suite ==="
echo ""

for file in samples/0*.rzn; do
    name=$(basename "$file" .rzn)
    echo -n "  [$name] "
    if $RAZENC $VERBOSE "$file" > /tmp/razenc_${name}.log 2>&1; then
        echo "✓ PASS"
        PASS=$((PASS + 1))
    else
        echo "✗ FAIL"
        cat /tmp/razenc_${name}.log
        FAIL=$((FAIL + 1))
    fi
    # Ignore dep_waits: they're from child processes
done

echo ""
echo "=== Results: $PASS passed, $FAIL failed, $((PASS + FAIL)) total ==="
exit $FAIL
