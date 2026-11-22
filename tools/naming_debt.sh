#!/usr/bin/env bash
# Count file-scope struct fields with mixed-case names in src/, and the four
# player sub-struct abbreviations as words in C sources/headers under src/tests/tools.
# Function bodies are skipped: private locals are not file interfaces, and GCC
# permits nested functions that Clang does not parse.
set -euo pipefail
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "usage: naming_debt.sh INCLUDE_FLAGS [CODE_ROOT]" >&2
    exit 2
fi
ROOT="$(cd "${2:-.}" && pwd -P)"
for directory in src tests tools; do
    if [ ! -d "$ROOT/$directory" ]; then
        echo "naming_debt.sh: missing $ROOT/$directory" >&2
        exit 2
    fi
done
mapfile -t sources < <(find "$ROOT/src" \( -name '*.c' -o -name '*.h' \) | sort)
if [ "${#sources[@]}" -eq 0 ]; then
    echo "naming_debt.sh: no C sources or headers under $ROOT/src" >&2
    exit 2
fi
if ! declarations=$(clang-query -c='set bind-root false' \
    -c='match fieldDecl(matchesName("(^|::)[^:]*[a-z][A-Z][^:]*$"), isExpansionInMainFile()).bind("field")' \
    "${sources[@]}" -- -x c $1 -Xclang -skip-function-bodies 2>&1); then
    printf '%s\n' "$declarations" >&2
    exit 2
fi
# clang-query can exit zero even after a front-end error.
if printf '%s\n' "$declarations" | grep -E '(^|:[0-9]+:[0-9]+: )(fatal )?error:' >/dev/null; then
    printf '%s\n' "$declarations" >&2
    exit 2
fi
fields=$(printf '%s\n' "$declarations" | awk '/^Match #/ { n++ } END { print n+0 }')
if words=$(grep -rhoE '\b(tPI|cPI|bTPI|cTPI)\b' "$ROOT/src" "$ROOT/tests" "$ROOT/tools" \
    --include='*.c' --include='*.h'); then
    :
else
    status=$?
    if [ "$status" -ne 1 ]; then exit "$status"; fi
fi
abbreviations=$(printf '%s\n' "$words" | awk 'NF { n++ } END { print n+0 }')
printf '%s\t%s\n' "$fields" "$abbreviations"
