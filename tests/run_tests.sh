#!/bin/bash

cd "$(dirname "$0")/.."
RAZENC="./razenc"
PASS=0
FAIL=0

run_one() {
    local rz="$1"
    local name
    name=$(basename "$rz" .rzn)
    echo -n "  [$name] "

    # Compile
    if ! $RAZENC "$rz" > /dev/null 2>&1; then
        echo "FAIL (compile)"
        FAIL=$((FAIL + 1))
        return
    fi

    # Link
    local obj="output/$name.o"
    local bin="/tmp/rzn_test_$name"
    if ! gcc -no-pie -o "$bin" "$obj" 2>/dev/null; then
        echo "FAIL (link)"
        FAIL=$((FAIL + 1))
        return
    fi

    # Run — any exit code is fine as long as it doesn't crash (signal)
    set +e
    "$bin" > /dev/null 2>&1
    local rc=$?
    set -e
    if [ $rc -ge 128 ]; then
        echo "FAIL (crashed with signal $((rc - 128)))"
        FAIL=$((FAIL + 1))
    else
        echo "PASS (exit $rc)"
        PASS=$((PASS + 1))
    fi
}

echo "Testing Razen-CPP compilation + execution"
echo ""

for f in tests/*.rzn; do
    run_one "$f"
done

echo ""
echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
