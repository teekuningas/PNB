#!/usr/bin/env bash
# compare.sh A B — for every run: identical, or the first frame and the sections that differ. A run that
# only one side recorded is named, never skipped: a comparison that quietly shrinks is not a comparison.
HERE="$(cd "$(dirname "$0")" && pwd)"
same=0
for f in "$1"/*.trace; do
    n=$(basename "$f" .trace)
    if [ ! -f "$2/$n.trace" ]; then echo "$n: only in $1"; continue; fi
    if cmp -s "$f" "$2/$n.trace"; then same=$((same + 1)); else echo "$n: $(python3 "$HERE/firstdiff.py" "$f" "$2/$n.trace")"; fi
done
for f in "$2"/*.trace; do
    n=$(basename "$f" .trace)
    [ -f "$1/$n.trace" ] || echo "$n: only in $2"
done
echo "identical: $same / $(ls "$1"/*.trace | wc -l)"
