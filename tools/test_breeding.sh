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
    native/source/breeding.c native/source/linux_roadmap.c native/source/nxpanded_costs.c native/source/nxpanded_hub_reference.c native/source/nxpanded_text.c native/source/linux_visual_settings.c native/tests/test_roadmap_toast.c -lm -o "$output"
"$output"
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/linux_visual_settings.c native/tests/test_visual_settings.c -o "$output"
"$output"

"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/nxpanded_text.c native/tests/test_nxpanded_text.c -lm -o "$output"
"$output"

"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/linux_sdl_filter.c native/tests/test_input_filter.c -o "$output"
"$output"

"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include native/tests/test_hub_hold.c -o "$output"
"$output"

"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/breeding.c native/source/nxpanded_costs.c native/tests/test_nxpanded_costs.c -o "$output"
"$output"

"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I native/include \
    native/source/nxpanded_hub_reference.c native/tests/test_nxpanded_hub_reference.c -o "$output"
"$output"
