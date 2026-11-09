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
ratchet "dead parameters (-Wunused-parameter)" "$dead_params" 31

# A non-static function with no prototype in any header is an edge in the include
# graph that no header records: nothing outside the file can see it declared, yet
# the linker exports it anyway (Unified DAG L3).
ratchet "functions with no prototype (src/)" "$no_prototype" 0

# Everything else -Wextra reports. Held at its current level so a new class of
# warning cannot arrive unnoticed under cover of the two counted above.
ratchet "other -Wall/-Wextra warnings" "$other_warnings" 0

# ---------------------------------------------------------------------------
# The include graph is the ownership graph. Until 2026-11-05 one 1,296-line header,
# globals.h, handed every type to every file (its last floor was 91 includers — 94 in
# fact: three path-qualified includes the count never matched). It was split by owner
# and deleted; every file now includes the header of every cluster it names. What that
# buys is measured here on the CLOSURE the compiler computes (gcc -MM), so an edge cannot
# hide behind a transitive include:
#   - the World's header reaches neither the legal world, the tick's input, the session
#     nor a controller's memory — code holding only world.h cannot name any of them;
#   - no engine file reaches a controller's memory, the session, the menus, the screen
#     or the keys. The engine is everything under src/game and src/physics except the
#     files named below — the controllers, the client and the assembly points — so a new
#     file is held to the engine's standard unless it is deliberately named here.
# ---------------------------------------------------------------------------
NOT_ENGINE="src/game/action_invocations.c src/game/game_frame.c src/game/game_screen.c src/game/game_setup.c src/game/ball.c src/game/player.c src/game/immutable_world.c"
FORBIDDEN_TO_ENGINE='/(client_input|ai_memory|session|menu_types|render|display|key_states)\.h$'
engine_files=$(find src/game src/physics -name '*.c' -not -path 'src/game/ai/*' -not -path 'src/game/ai_pure/*' | sort)
engine_leaks=0
for f in $engine_files; do
    case " $NOT_ENGINE " in *" $f "*) continue ;; esac
    # A closure that cannot be computed is a failure, never an empty (and so clean) one.
    if ! deps=$(gcc -MM $IDIR "$f" 2>/dev/null); then
        engine_leaks=$((engine_leaks + 1))
        problems="${problems}  $f: gcc -MM could not compute its include closure
"
        continue
    fi
    reached=$(echo "$deps" | tr ' \\' '\n\n' | grep -E "$FORBIDDEN_TO_ENGINE")
    if [ -n "$reached" ]; then
        engine_leaks=$((engine_leaks + 1))
        problems="${problems}  $f can see: $(echo $reached | tr '\n' ' ')
"
    fi
done
if world_deps=$(echo '#include "world.h"' | gcc -MM $IDIR -x c - 2>/dev/null); then
    world_reaches=$(echo "$world_deps" | tr ' \\' '\n\n' | grep -cE '/(rules_state|intents|session|client_input|ai_memory)\.h$')
else
    world_reaches=1
    problems="${problems}  world.h: gcc -MM could not compute its include closure
"
fi
ratchet "engine files that can see past the engine" "$engine_leaks" 0
ratchet "headers the World's header can see past it" "$world_reaches" 0

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
ratchet "files awaiting the function-quality audit" "$unaudited" 49 # the AI controllers audited at the controller-eviction slice, as their deferral said

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
# The refactor's own frontier, and it is now AT ZERO. ActionFlags was the pre-intent
# channel: a struct of flags that producers wrote and execution read. The
# controller-symmetry redesign dissolved it into per-team value messages, slice by
# slice — the fielder-movement slice took the whole catching half (cTAF.move[]), the
# batter-selection slice took choose_batter (63 → 53) and the two batter-angle triggers
# (53 → 33), and the swing slice took the last field there was, the swing's phase, when
# the swing became a declared power and a declared elevation (33 → 0).
#
# The row does NOT retire with the struct. It stays at zero as a floor, because what it
# has really been measuring all along is "a producer writes engine state directly", and
# that is the thing that must not come back. A future action added as a flag instead of
# a message fails the build on this line.
# ---------------------------------------------------------------------------
action_flags_lines=$(grep -rE 'aF\.|ActionFlags' src --include='*.c' --include='*.h' | wc -l)
ratchet "lines touching ActionFlags" "$action_flags_lines" 0

# ---------------------------------------------------------------------------
# Zero const-casts — the first principle's own claim, and until 2026-10-27 nothing counted it: the draw
# code reached the World's camera through `(CameraState*)&(stateInfo->match->cameraState)` on a const
# StateInfo, and every guardrail stayed green. A cast to a struct pointer is how const is thrown away, so
# this counts every pointer cast in src/ that is not the result of an allocation; there are none, and
# the floor is a floor.
# ---------------------------------------------------------------------------
pointer_casts=$(grep -rnE '\(\s*(const\s+)?[A-Za-z_][A-Za-z_0-9]*\s*\*\s*\)' src --include='*.c' --include='*.h' |
    grep -vE '\(\s*(const\s+)?(void|char)\s*\*\s*\)' | grep -vE '\*\s*\)\s*(malloc|calloc|realloc)\(' |
    grep -vE '^[^:]+:[0-9]+:[[:space:]]*//' | wc -l)
ratchet "pointer casts other than allocations" "$pointer_casts" 0

# ---------------------------------------------------------------------------
# No read-back — the fourth law. A controller reads the world to know what is legal and to draw:
# what a body is doing (catching_action.kind, a ball in the air) is the world. What a producer
# DECLARED, once the engine holds it, is not — the windup's power and aim, a throw's target and
# power (catching_action.as), the swing's values (at_bat.swing). A controller that reads
# its own declaration back is keyed to the engine's copy, which is a wire's delay late; the human
# widgets keep their own copy instead and the AI its plan. Held at 0 in the controllers' files.
# ---------------------------------------------------------------------------
controller_readbacks=$(grep -nE 'catching_action\.as|at_bat\.swing' src/game/action_invocations.c \
    src/game/ai/*.c | grep -vE '^[^:]+:[0-9]+:[[:space:]]*//' | wc -l)
ratchet "controller reads of declared values" "$controller_readbacks" 0

# Field declarations come from the compiler, not a regex over comments or types.
# The abbreviation row includes tests and the golden projection: every caller
# must carry a renamed member, not just the production files.
if ! naming_counts=$(bash tools/naming_debt.sh "$IDIR"); then
    echo "guardrails.sh: could not measure naming debt" >&2
    exit 2
fi
read -r camel_fields player_abbreviations <<< "$naming_counts"
ratchet "mixed-case file-scope struct fields" "$camel_fields" 195
ratchet "player sub-struct abbreviation words" "$player_abbreviations" 881

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
