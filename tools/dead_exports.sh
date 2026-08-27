#!/usr/bin/env bash
#
# dead_exports.sh — the exported surface of src/, measured by the linker.
#
# Run it with `make dead_exports` (and as the last step of `make check`).
#
# What it counts: functions defined in src/ that NO other translation unit
# references — in any of the builds we produce (the render build, the unit build,
# and the NO_RENDER build the four test tiers link). Two different defects share
# that one measurement, and the report says which is which:
#
#   no in-file caller  → dead code. Delete it.
#   file-local only    → the header exports an edge that does not exist. Make it
#                        static, and the include graph stops lying.
#
# Why this is a separate script from guardrails.sh. That sweep is the compiler
# front end alone (`gcc -fsyntax-only`, ~6 s, no objects) and it is deliberately the
# cheapest orientation there is. This question cannot be answered there: whether
# anything outside a file uses a symbol is a fact about the whole link, so the
# measurement needs real objects. `make` supplies them from the same object lists
# the five tiers link, so a stale obj/ can never be measured — but it does mean this
# row costs a build, which is why it rides with `make check` rather than with the
# six-second sweep.
#
# Why it exists at all: -Wmissing-prototypes already fails a function no header
# declares. This is the mirror defect — a header that declares a function nobody
# outside calls — and until this script nothing could see it. Both defects were
# found by hand before that, and late: `core/geometry.c` spent an unknown length of
# time four-fifths dead behind its own header, and three dead accessors survived the
# referee.c pilot audit. A signature that names a parameter it never reads
# over-claims its edges; a header that exports a function nothing imports
# over-claims its module's interface. Same principle, one level up.
#
# Usage: tools/dead_exports.sh <object files...>

set -u

cd "$(dirname "$0")/.." || exit 2

# The floor. Lower it in the same commit that improves the number — see guardrails.sh
# for why an improvement is also a failure.
#
# 3 -> 2 when the fielder-movement slice deleted move_controlled_player_to_location
# along with the AI's key puppeteering. The two that remain still live in files a
# scheduled slice rewrites: catching_ai.c splits further along the law-5 seam, and
# throwing_system.c dissolves across the rest of the movement work and the swing slice
# (update_controlled_player_speed dies with the human key path). Making them static now
# is churn on code that is about to move, so they stay as the row's live examples.
FLOOR=2

# `main` is referenced by the C runtime, not by any object we compile.
EXEMPT_SYMBOLS=" main "

if [ "$#" -eq 0 ]; then
    echo "dead_exports.sh: expected the object files as arguments (make passes them)" >&2
    exit 2
fi

# nm -A prefixes every line with its object file, so one pass over all objects can be
# parsed without tracking which file the reader is inside.
defined="$(nm -A --defined-only "$@" 2>/dev/null | awk -F: '{
    n = split($2, f, " ")
    if (n == 3 && f[2] == "T") print f[3] "\t" $1
}' | sort -u)"

referenced="$(nm -A --undefined-only "$@" 2>/dev/null | awk -F: '{
    n = split($2, f, " ")
    if (n == 2 && f[1] == "U") print f[2]
}' | sort -u)"

# Map an object back to the source that produced it: obj/<build>/<path>.o, where
# <path> is relative to src/ for the game and to the repo root for the test tiers.
source_of() {
    local rel="${1#obj/*/}"
    rel="${rel%.o}.c"
    case "$rel" in
        tests/*) printf '%s\n' "$rel" ;;
        *) printf '%s\n' "src/$rel" ;;
    esac
}

report=""
count=0

while IFS=$'\t' read -r sym obj; do
    [ -n "$sym" ] || continue
    case "$EXEMPT_SYMBOLS" in *" $sym "*) continue ;; esac
    printf '%s\n' "$referenced" | grep -qx -- "$sym" && continue

    src="$(source_of "$obj")"
    # Only src/ is ours to hold to this standard; a helper local to a test tier is a
    # different question, and the tiers link their own runners.
    case "$src" in src/*) ;; *) continue ;; esac
    [ -f "$src" ] || continue

    # The amalgamated third-party headers (stb_image, miniaudio) define hundreds of
    # functions inside whatever .c includes them, and external/ is not ours to fix.
    # A symbol counts as ours only if the .c itself defines it at file scope.
    grep -qE "^[A-Za-z_].*[^A-Za-z0-9_]${sym}[[:space:]]*\(" "$src" || continue

    # The definition line always matches, so a second match means an in-file caller —
    # which is the difference between "delete it" and "make it static".
    if [ "$(grep -cE "[^A-Za-z0-9_]?${sym}[[:space:]]*\(" "$src")" -gt 1 ]; then
        verdict="file-local only — make it static"
    else
        verdict="no in-file caller — delete it"
    fi

    report="${report}$(printf '  %-42s %-38s %s' "$src" "$sym" "$verdict")
"
    count=$((count + 1))
done <<< "$(printf '%s\n' "$defined" | awk -F'\t' '!seen[$1]++')"

echo
echo "  PNB exported surface — functions in src/ that nothing outside their own file uses"
echo
if [ -n "$report" ]; then
    printf '%s' "$report"
else
    echo "  none"
fi
echo
printf '  %-42s %6s   ' "exported but unused elsewhere" "$count"

if [ "$count" -gt "$FLOOR" ]; then
    echo "REGRESSED (floor $FLOOR)"
    echo
    echo "  A new export nothing imports. Either give it a caller, make it static, or delete it."
    exit 1
elif [ "$count" -lt "$FLOOR" ]; then
    echo "IMPROVED — lower FLOOR to $count in this commit"
    echo
    echo "  tools/dead_exports.sh carries the floor; lowering it here is what makes the new"
    echo "  level the one that holds from now on."
    exit 1
fi

echo "ok"
echo
exit 0
