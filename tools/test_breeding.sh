#!/usr/bin/env sh
# Host-side regression tests; no Switch SDK or game client required.
set -eu
cd "$(dirname "$0")/.."
output="${TMPDIR:-/tmp}/pokemmo-breeding-tests-$$"
trap 'rm -f "$output"' EXIT HUP INT TERM
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/breeding.c native/tests/test_breeding_priority.c -o "$output"
"$output"
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/breeding.c native/source/linux_roadmap.c native/source/nxpanded_text.c native/tests/test_roadmap_toast.c -lm -o "$output"
"$output"

"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/nxpanded_text.c native/tests/test_nxpanded_text.c -lm -o "$output"
"$output"
