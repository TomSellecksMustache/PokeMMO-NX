#pragma once
#include "linux_visual_effects.h"

// All settings use controller buttons; each visible effect may be enabled separately.
#define LINUX_VISUAL_TABS 2u
static inline unsigned linuxVisualMenuRows(unsigned tab) { return tab < 2u ? 6u : 0u; }
static inline unsigned linuxVisualMenuEffect(unsigned tab, unsigned row) {
    static const unsigned map[2][6] = {
        {VISUAL_COLOR_GRADE, VISUAL_VIGNETTE, VISUAL_VIBRANCE, VISUAL_CONTRAST, VISUAL_SHADOWS, VISUAL_HIGHLIGHTS},
        {VISUAL_SOFT_PIXELS, VISUAL_TILT_SHIFT, VISUAL_BLOOM, VISUAL_FXAA, VISUAL_SHARPEN, VISUAL_HAZE}
    };
    return tab < 2u && row < 6u ? map[tab][row] : VISUAL_EFFECT_COUNT;
}
static inline const char *linuxVisualMenuDescription(unsigned effect) {
    switch (effect) {
        case VISUAL_COLOR_GRADE: return "Warmer natural tones.";
        case VISUAL_VIGNETTE: return "Darkens corners for focus.";
        case VISUAL_VIBRANCE: return "Strengthens muted colours.";
        case VISUAL_CONTRAST: return "Improves separation of light and dark.";
        case VISUAL_SHADOWS: return "Deepens shadow areas.";
        case VISUAL_HIGHLIGHTS: return "Smooths very bright highlights.";
        case VISUAL_SOFT_PIXELS: return "Softens sharp pixel edges.";
        case VISUAL_TILT_SHIFT: return "Blurs the top and bottom edges.";
        case VISUAL_BLOOM: return "Adds glow around bright areas.";
        case VISUAL_FXAA: return "Smooths jagged object edges.";
        case VISUAL_SHARPEN: return "Restores fine edges after softening.";
        case VISUAL_HAZE: return "Adds a subtle atmospheric veil.";
        default: return "";
    }
}
