#!/usr/bin/env bash
# compare.sh A B — for every run: identical, or the first frame and the sections that differ. A run that
# only one side recorded is named, never skipped: a comparison that quietly shrinks is not a comparison.
set -euo pipefail
if [ "$#" -ne 2 ] || [ ! -d "$1" ] || [ ! -d "$2" ]; then
    echo "usage: compare.sh A B (two recording directories)" >&2
    exit 2
fi
HERE="$(cd "$(dirname "$0")" && pwd)"
shopt -s nullglob
left=("$1"/*.trace)
right=("$2"/*.trace)
if [ "${#left[@]}" -eq 0 ] || [ "${#right[@]}" -eq 0 ]; then
    echo "compare.sh: both recordings must contain traces" >&2
    exit 2
fi
same=0
failed=0
for f in "${left[@]}"; do
    n=$(basename "$f" .trace)
    if [ ! -f "$2/$n.trace" ]; then echo "$n: only in $1"; failed=1; continue; fi
    if detail=$(python3 "$HERE/firstdiff.py" "$f" "$2/$n.trace"); then
        same=$((same + 1))
    else
        echo "$n: $detail"
        failed=1
    fi
done
for f in "${right[@]}"; do
    n=$(basename "$f" .trace)
    if [ ! -f "$1/$n.trace" ]; then echo "$n: only in $2"; failed=1; fi
done
echo "identical: $same / ${#left[@]}"
exit "$failed"
