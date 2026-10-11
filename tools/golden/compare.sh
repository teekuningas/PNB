#!/usr/bin/env bash
# compare.sh A B — for every run: identical, or the first frame and the sections that differ.
HERE="$(cd "$(dirname "$0")" && pwd)"
same=0
for f in "$1"/*.trace; do
    n=$(basename "$f" .trace)
    if cmp -s "$f" "$2/$n.trace"; then same=$((same + 1)); else echo "$n: $(python3 "$HERE/firstdiff.py" "$f" "$2/$n.trace")"; fi
done
echo "identical: $same / $(ls "$1"/*.trace | wc -l)"
