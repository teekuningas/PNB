#!/usr/bin/env bash
#
# guardrails.sh — the numbers this codebase has decided may only go down.
#
# Run it with `make guardrails`. Each row below is a measurement plus a recorded
# floor. Measuring above the floor fails the build; measuring below it fails too,
# and says so — a number that improved is a floor that should be lowered in the
# same commit, so the ratchet cannot silently slacken again later.
#
# Why this exists: a figure quoted in prose relies on someone remembering to
# re-measure it. The project's working rule is that violating a principle should fail
# loudly instead of relying on vigilance. This turns the figures into build facts.
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
VOCAB_LEDGER="tools/rules_vocabulary.tsv"

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
# dependency DAG — the mirror of the missing const. A signature is meant to be the
# function's complete, honest edge list.
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
# hands everything to everyone. The planned foundation.h split drives this to zero.
# ---------------------------------------------------------------------------
globals_includers=$(grep -rl '#include "globals.h"' src tests | wc -l)
# 84 → 85 on 2026-08-24, deliberately: a new contract test that names MatchSession and a declaration
# type. Every new file naming a World type costs one here until the foundation.h split lands, and the
# honest options are to pay it or to hide the dependency behind a transitive include — which would
# leave the number looking better while the coupling stayed exactly the same. Paid, not hidden.
#
# 85 → 87 on 2026-08-27, deliberately, for the fielder-movement slice: the engine mover's header
# (it takes a MatchSession) and the contract test that drives it. The alternative was to leave the
# mover inside throwing_system.c, where the key-driven version lived — a file about the throw
# holding the walk, which is what made the movement code hard to find in the first place. The .c
# takes globals.h through its own header, as every other file under src/game/actions does, so the
# cost here is the honest one and not one include padded onto it.
#
# 87 → 88 the same day: the scripted-human test for movement, which names MatchSession exactly as
# every other file in that tier does.
#
# 88 → 89 on 2026-08-28: the batter-selection contract tests, which build the §12 window and name
# MatchSession, IntentMessage and JokerStatus to do it. The rule they exercise, rules_batting_order.c,
# includes NOTHING and costs zero here — the price is the test that drives the engine, not the rule.
ratchet "files including globals.h" "$globals_includers" 89

# ---------------------------------------------------------------------------
# Function-quality audit coverage. Two separate things are checked:
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
# The rules-vocabulary ledger. A handful of rulebook sections define no consequence —
# they define the WORDS every other rule is written in. When one of those is not written
# down as a predicate, each caller improvises a proxy, the proxies disagree, and one of
# them is wrong somewhere nobody looked. That is bug #8 exactly. This counts the
# definitions not yet represented as a pure predicate in rules_pure/; it may only fall,
# and the way to fall it is to convert a definition while fixing the rule that needs it.
# ---------------------------------------------------------------------------
vocab_pending=$(awk -F'\t' '!/^#/ && NF > 1 && $2 != "predicate"' "$VOCAB_LEDGER" 2>/dev/null | wc -l)
ratchet "rule definitions not yet a predicate" "$vocab_pending" 11

# ---------------------------------------------------------------------------
# The refactor's own frontier. ActionFlags is the pre-intent channel: a struct of
# flags that producers write and execution reads. The controller-symmetry redesign
# dissolves it into per-team value messages, and that redesign is defined as complete
# exactly when `grep -r ActionFlags src/` returns nothing. Counting
# the lines that still touch it makes that progress a build fact like every other
# number here — and, more importantly, makes it FAIL if a new intent is ever added to
# ActionFlags instead of to the channel. The fielder-movement slice took cTAF.move[]
# and with it the whole catching half, so what is left is batting-only. Each remaining
# slice lowers this floor further. The batter-selection slice took choose_batter (63 → 53);
# what remains is the swing and the two batter-angle triggers, which go with the swing
# slice and the angle step — at which point the row retires at 0.
# ---------------------------------------------------------------------------
action_flags_lines=$(grep -rE 'aF\.|ActionFlags' src --include='*.c' --include='*.h' | wc -l)
ratchet "lines touching ActionFlags" "$action_flags_lines" 53

# ---------------------------------------------------------------------------
# The docs depend on the code; the code must NOT depend on the docs. That edge is
# one-way on purpose — the code is the thing that is true, and the docs describe it.
# A comment citing a doc file or a doc section number is a back-edge, and it rots
# silently: the docs get restructured, the citation still reads like live guidance,
# and it now points at nothing. That is not hypothetical — a 2026-08-24 sweep found
# 102 such references across 36 files, some of which had been dangling for months
# because nothing could see them.
#
# So: a comment must SAY the thing, not cite where the thing is written. Rulebook
# sections (a bare `§12`, `§24`) are exempt and always fine — those are pesäpallo's
# own stable identifiers and are domain knowledge, not a dependency on our prose.
# What is forbidden is a doc FILENAME or a dotted `§N.M` section number.
#
# The pattern is ANY `*.md`, not a list of the docs we happen to have. A hardcoded list
# only sees back-edges to files someone remembered to add: it sat at zero while
# `sim_observers.h` cited an AI-notes markdown file that has never existed in this repo.
# ---------------------------------------------------------------------------
doc_backrefs=$(
    grep -rEn '[A-Za-z0-9_]+\.md|§[0-9]+\.[0-9]+' \
        src tests tools Makefile --include='*.c' --include='*.h' --include='*.sh' --include='*.tsv' 2>/dev/null |
        grep -v 'RULES_OFFICIAL\.md' | wc -l
)
ratchet "references from code into the docs" "$doc_backrefs" 0

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
