#!/usr/bin/env bash
#
# guardrails.sh — the numbers this codebase has decided may only go down.
#
# Run it with `make guardrails`. Each row below is a measurement plus a recorded
# floor. Measuring above the floor fails the build; measuring below it fails too,
# and says so — a number that improved is a floor that should be lowered in the
# same commit, so the ratchet cannot silently slacken again later.
#
# Why this exists: docs PNB/PLAN.md tracks each of these as a figure in prose, and a
# figure in prose relies on someone remembering to re-measure it. §3 rule 6 asks that
# violating a principle fail loudly instead. This turns the figures into build facts.
#
# The commands here are the DEFINITION of each number. When a doc quotes one, it is
# quoting this script, so "the number changed" can always be re-derived rather than
# remembered.
#
# Usage: tools/guardrails.sh "<compiler include flags>"

set -u

# Every path below is repo-relative, so run from the repo root whoever invoked us and
# from wherever. (No `set -e`: `grep -c` exits 1 on a count of zero, which is the
# normal, healthy case for most of these rows.)
cd "$(dirname "$0")/.." || exit 2

IDIR="${1:-}"
if [ -z "$IDIR" ]; then
    echo "guardrails.sh: expected the compiler include flags as \$1 (make passes \$(IDIR))" >&2
    exit 2
fi

AUDIT_LEDGER="tools/function_quality_audit.tsv"

failures=0
rows=""
problems=""

# Record one ratchet row. A measurement equal to its floor is the normal, quiet case.
ratchet() {
    local name="$1" measured="$2" floor="$3" status
    if [ "$measured" -gt "$floor" ]; then
        status="REGRESSED (floor $floor)"
        failures=$((failures + 1))
    elif [ "$measured" -lt "$floor" ]; then
        status="IMPROVED — lower the floor to $measured in this commit"
        failures=$((failures + 1))
    else
        status="ok"
    fi
    rows="${rows}$(printf '  %-42s %6s   %s' "$name" "$measured" "$status")
"
}

# ---------------------------------------------------------------------------
# One compiler pass feeds three of the rows. -fsyntax-only means no objects, no
# linking and no OpenGL — it is the front end alone, which is where all three of
# these warnings are raised. The pass is deliberately the RENDER build: -DNO_RENDER
# stubs out bodies and reports parameters as unused that the real build uses.
# Warnings raised inside external/ (miniaudio, stb_image) are not ours to fix, so
# only diagnostics whose file is under src/ are counted.
# ---------------------------------------------------------------------------
warnings="$(
    find src -name '*.c' | sort |
        xargs -n1 gcc -fsyntax-only $IDIR -Wall -Wextra -Wmissing-prototypes 2>&1 |
        grep '^src/.*warning:'
)"

count_warning() { printf '%s\n' "$warnings" | grep -c -- "$1"; }

dead_params=$(count_warning '\[-Wunused-parameter\]')
no_prototype=$(count_warning '\[-Wmissing-prototypes\]')
other_warnings=$(($(printf '%s\n' "$warnings" | grep -c 'warning:') - dead_params - no_prototype))

# A signature that names a parameter it never reads over-claims its edge in the
# dependency DAG — the mirror of the missing const (ARCHITECTURE_VISION.md §2.1).
ratchet "dead parameters (-Wunused-parameter)" "$dead_params" 34

# A non-static function with no prototype in any header is an edge in the include
# graph that no header records: nothing outside the file can see it declared, yet
# the linker exports it anyway (Unified DAG L3).
ratchet "functions with no prototype (src/)" "$no_prototype" 0

# Everything else -Wextra reports. Held at its current level so a new class of
# warning cannot arrive unnoticed under cover of the two counted above.
ratchet "other -Wall/-Wextra warnings" "$other_warnings" 1

# ---------------------------------------------------------------------------
# The include graph should become the ownership graph: a file that includes only
# what it owns physically cannot name RefereeState. Today one 977-line header
# hands everything to everyone. PLAN.md §6.2 drives this number to zero.
# ---------------------------------------------------------------------------
globals_includers=$(grep -rl '#include "globals.h"' src tests | wc -l)
ratchet "files including globals.h" "$globals_includers" 84

# ---------------------------------------------------------------------------
# Function-quality audit coverage (PLAN.md §3.3). Two separate things are checked:
# the ledger must describe the tree exactly (a hard error — this is what stops a
# file being silently skipped), and the unaudited count is the ratchet.
# ---------------------------------------------------------------------------
ledger_rows="$(grep -v '^#' "$AUDIT_LEDGER" | grep -v '^[[:space:]]*$')"
ledger_paths="$(printf '%s\n' "$ledger_rows" | cut -f2 | sort)"
tree_paths="$(find src -name '*.c' | sort)"

missing_from_ledger="$(comm -23 <(printf '%s\n' "$tree_paths") <(printf '%s\n' "$ledger_paths"))"
stale_in_ledger="$(comm -13 <(printf '%s\n' "$tree_paths") <(printf '%s\n' "$ledger_paths"))"

if [ -n "$missing_from_ledger" ]; then
    failures=$((failures + 1))
    problems="${problems}  not classified in $AUDIT_LEDGER — every src/*.c needs a row there:
$(printf '%s\n' "$missing_from_ledger" | sed 's/^/    /')
"
fi
if [ -n "$stale_in_ledger" ]; then
    failures=$((failures + 1))
    problems="${problems}  listed in $AUDIT_LEDGER but no longer in the tree:
$(printf '%s\n' "$stale_in_ledger" | sed 's/^/    /')
"
fi

audited=$(printf '%s\n' "$ledger_rows" | cut -f1 | grep -c '^audited$')
unaudited=$(($(printf '%s\n' "$ledger_rows" | wc -l) - audited))
ratchet "files awaiting the function-quality audit" "$unaudited" 52

# ---------------------------------------------------------------------------
# The refactor's own frontier (PLAN.md §5.10 / §8.0). ActionFlags is the pre-intent
# channel: a struct of flags that producers write and execution reads. The
# controller-symmetry redesign dissolves it into per-team value messages, and §8.0
# defines "§5.10 complete" as `grep -r ActionFlags src/` returning nothing. Counting
# the lines that still touch it makes that progress a build fact like every other
# number here — and, more importantly, makes it FAIL if a new intent is ever added to
# ActionFlags instead of to the channel. Each remaining slice lowers this floor:
# 1b (the four pure commands + pitch/throw), 2 (cTAF.move[]), 3 (batter select,
# free walk, swing angle), 4 (the swing) — at which point the row retires at 0.
# ---------------------------------------------------------------------------
action_flags_lines=$(grep -rE 'aF\.|ActionFlags' src --include='*.c' --include='*.h' | wc -l)
ratchet "lines touching ActionFlags (§5.10)" "$action_flags_lines" 142

# ---------------------------------------------------------------------------
# Formatting is not a ratchet — it is settled, and `make format` restores it.
# ---------------------------------------------------------------------------
format_deviations=$(
    find src tests \( -name '*.c' -o -name '*.h' \) ! -name 'miniaudio.h' ! -name 'stb_image.h' |
        xargs clang-format --output-replacements-xml | grep -c '<replacement '
)
ratchet "clang-format deviations" "$format_deviations" 0

# ---------------------------------------------------------------------------

echo
echo "  PNB guardrails — numbers that may only go down"
echo
printf '  %-42s %6s   %s\n' "ratchet" "now" "status"
printf '%s' "$rows"
echo
echo "  function-quality audit: $audited of $(printf '%s\n' "$ledger_rows" | wc -l | tr -d ' ') files audited"
if [ -n "$problems" ]; then
    echo
    printf '%s' "$problems"
fi

if [ "$failures" -ne 0 ]; then
    echo
    echo "  $failures guardrail(s) need attention."
    echo "  A ratchet that IMPROVED is not a failure to undo: lower its floor in tools/guardrails.sh"
    echo "  in the same commit, so the new level is the one that holds from now on."
    exit 1
fi

echo
echo "  All guardrails hold."
exit 0
