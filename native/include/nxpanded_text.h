#pragma once
#include <stddef.h>
#include <stdint.h>
// Pixel displacement for a one-way, pause, reverse (non-jumping) marquee.
float nxTextMarqueeOffset(size_t characters, float advance, float available_width, uint64_t elapsed_ns);
// Find the next line boundary for simple menu descriptions.
size_t nxTextWrapLine(const char *s, size_t columns);
