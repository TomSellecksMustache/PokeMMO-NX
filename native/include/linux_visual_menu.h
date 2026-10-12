#pragma once
#include "linux_visual_effects.h"

// Fixed, controller-only two-tab layout. Do not add effects without a real
// performance reason: everyday adjustments first, texture-heavy effects second.
#define LINUX_VISUAL_TABS 2u
static inline unsigned linuxVisualMenuRows(unsigned tab) {
    return tab == 0 ? 2u : (tab == 1 ? 3u : 0u);
}
static inline unsigned linuxVisualMenuEffect(unsigned tab, unsigned row) {
    if (tab == 0) {
        if (row == 0) return VISUAL_COLOR_GRADE;
        if (row == 1) return VISUAL_VIGNETTE;
    } else if (tab == 1) {
        if (row == 0) return VISUAL_SOFT_PIXELS;
        if (row == 1) return VISUAL_TILT_SHIFT;
        if (row == 2) return VISUAL_BLOOM;
    }
    return VISUAL_EFFECT_COUNT;
}
static inline const char *linuxVisualMenuDescription(unsigned effect) {
    switch (effect) {
        case VISUAL_COLOR_GRADE: return "Gently adjusts warmth and colour.";
        case VISUAL_VIGNETTE:    return "Subtly darkens the corners.";
        case VISUAL_SOFT_PIXELS: return "Blends hard edges; may soften game text.";
        case VISUAL_TILT_SHIFT:  return "Softens the top and bottom of the scene.";
        case VISUAL_BLOOM:       return "Adds a faint glow around bright areas.";
        default:                 return "";
    }
}
