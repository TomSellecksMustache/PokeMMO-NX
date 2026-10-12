#include "nxpanded_text.h"
#include <math.h>
#include <string.h>

// Font and row widths are measured in visible pixels. No wrap/travel when text fits.
float nxTextMarqueeOffset(size_t characters, float advance, float available_width, uint64_t elapsed_ns) {
    if (advance <= 0 || available_width <= 0) return 0;
    const float overshoot = (float)characters * advance - available_width;
    if (overshoot <= 0) return 0;
    const double pause_seconds = 1.0, speed = 34.0;
    const double one_way = overshoot / speed;
    const double period = 2.0 * (one_way + pause_seconds);
    const double seconds = fmod((double)elapsed_ns / 1000000000.0, period);
    double distance = 0;
    if (seconds < pause_seconds) distance = 0;
    else if (seconds < pause_seconds + one_way) distance = (seconds - pause_seconds) * speed;
    else if (seconds < 2.0 * pause_seconds + one_way) distance = overshoot;
    else distance = overshoot - (seconds - 2.0 * pause_seconds - one_way) * speed;
    if (distance < 0) distance = 0;
    if (distance > overshoot) distance = overshoot;
    return (float)distance;
}

// Returns the number of bytes of the next line, without cutting a word when possible.
// ASCII spaces suffice for the menu text; glyph codes remain owned by the renderer.
size_t nxTextWrapLine(const char *s, size_t columns) {
    if (!s || !*s || !columns) return 0;
    size_t n = strlen(s);
    if (n <= columns) return n;
    size_t cut = columns;
    while (cut && s[cut] != ' ' && s[cut] != '\t') --cut;
    return cut ? cut : columns;
}
