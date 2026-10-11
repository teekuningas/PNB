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
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:?usage: record.sh OUTDIR [GITREF]}"; REF="${2:-}"
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
case "$OUT/" in "$REPO"/*) echo "record.sh: OUTDIR must be outside the repository" >&2; exit 2 ;; esac
TREE="$REPO"
if [ -n "$REF" ]; then
    TREE="$OUT/.tree"
    rm -rf "$TREE"; git -C "$REPO" worktree prune; git -C "$REPO" worktree add -f -q "$TREE" "$REF"
fi
cd "$TREE"
if [ ! -f tools/golden/projection.h ]; then
    echo "record.sh: ${REF:-the working tree} has no tools/golden — the harness arrived with the channel slice" >&2
    [ -n "$REF" ] && { cd "$REPO"; git worktree remove --force "$TREE"; }
    exit 2
fi
nix develop "$REPO" --command make -s scripted_runner >/dev/null 2>&1
MK="make -s --no-print-directory -f Makefile -f tools/golden/print.mk"
OBJS=$(nix develop "$REPO" --command $MK golden-print-objs 2>/dev/null)
FLAGS=$(nix develop "$REPO" --command $MK golden-print-flags 2>/dev/null)
nix develop "$REPO" --command gcc tools/golden/golden.c $OBJS $FLAGS -o "$OUT/golden"
RUNS="sim0 sim1 sim2 sim3 sim4 sim5 $(for i in $(seq 0 11); do printf "catch%s bat%s h2h%s " $i $i $i; done)"
for r in $RUNS; do ("$OUT/golden" "$OUT" "$r" > "$OUT/$r.log" 2>&1) & done
wait
if [ -n "$REF" ]; then cd "$REPO"; git worktree remove --force "$TREE"; fi
echo "recorded $(ls "$OUT"/*.trace | wc -l) runs into $OUT"
