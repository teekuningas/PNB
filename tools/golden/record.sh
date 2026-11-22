#!/usr/bin/env bash
# Record the golden traces of a tree.
#
#   tools/golden/record.sh OUTDIR [GITREF]
#
# Builds tools/golden/golden.c against the scripted tier's objects of GITREF (default: the working tree)
# and plays every run into OUTDIR/<run>.trace. OUTDIR must be outside the repository: results are never
# written into the code tree. A GITREF is checked out into a temporary worktree inside OUTDIR, so its own
# projection.h — the one that matches its structs — is the one used.
#
# The workflow for a behaviour-preserving change:
#   tools/golden/record.sh /tmp/g/before HEAD
#   ...change the code, and projection.h if a struct moved...
#   tools/golden/record.sh /tmp/g/after
#   tools/golden/compare.sh /tmp/g/before /tmp/g/after
set -euo pipefail
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "usage: record.sh OUTDIR [GITREF]" >&2
    exit 2
fi
REPO="$(cd "$(dirname "$0")/../.." && pwd -P)"
OUT="$1"; REF="${2:-}"
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd -P)"
case "$OUT/" in "$REPO"/*) echo "record.sh: OUTDIR must be outside the repository" >&2; exit 2 ;; esac
shopt -s nullglob dotglob
contents=("$OUT"/*)
if [ "${#contents[@]}" -ne 0 ]; then
    echo "record.sh: OUTDIR must be empty (old results must not masquerade as this run)" >&2
    exit 2
fi
TREE="$REPO"
worktree_added=0
cleanup() {
    local status=$?
    trap - EXIT
    set +e
    local pid
    for pid in $(jobs -pr); do kill "$pid"; done
    wait
    if [ "$worktree_added" -eq 1 ]; then
        if ! git -C "$REPO" worktree remove --force "$TREE"; then
            echo "record.sh: could not remove temporary worktree $TREE" >&2
            status=2
        fi
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
if [ -n "$REF" ]; then
    TREE="$OUT/.tree"
    git -C "$REPO" worktree add --detach -q "$TREE" "$REF"
    worktree_added=1
fi
cd "$TREE"
if [ ! -f tools/golden/projection.h ]; then
    echo "record.sh: ${REF:-the working tree} has no tools/golden — the harness arrived with the channel slice" >&2
    exit 2
fi
if ! nix develop "$REPO" --command make -s scripted_runner > "$OUT/build.log" 2>&1; then
    cat "$OUT/build.log" >&2
    exit 2
fi
MK="make -s --no-print-directory -f Makefile -f tools/golden/print.mk"
OBJS=$(nix develop "$REPO" --command $MK golden-print-objs)
FLAGS=$(nix develop "$REPO" --command $MK golden-print-flags)
nix develop "$REPO" --command gcc tools/golden/golden.c $OBJS $FLAGS -o "$OUT/golden"
RUNS="sim0 sim1 sim2 sim3 sim4 sim5 $(for i in $(seq 0 11); do printf "catch%s bat%s h2h%s " $i $i $i; done)"
# The other two modes, if this tree's harness knows them (it has since the contest and super-inning runs).
if grep -q supsim tools/golden/golden.c; then
    RUNS="$RUNS $(for i in $(seq 0 5); do printf "supsim%s suph2h%s hrsim%s hrh2h%s " $i $i $i $i; done)"
fi
pids=()
runs=()
for r in $RUNS; do
    "$OUT/golden" "$OUT" "$r" > "$OUT/$r.log" 2>&1 &
    pids+=("$!")
    runs+=("$r")
done
failures=0
for i in "${!pids[@]}"; do
    r="${runs[$i]}"
    if ! wait "${pids[$i]}"; then
        echo "record.sh: $r failed — see $OUT/$r.log" >&2
        failures=$((failures + 1))
    elif ! python3 "$REPO/tools/golden/firstdiff.py" "$OUT/$r.trace" "$OUT/$r.trace"; then
        echo "record.sh: $r has no successful trace — see $OUT/$r.log" >&2
        failures=$((failures + 1))
    fi
done
if [ "$failures" -ne 0 ]; then
    echo "record.sh: $failures run(s) failed; recording is not a baseline" >&2
    exit 1
fi
echo "recorded ${#runs[@]} runs into $OUT"
