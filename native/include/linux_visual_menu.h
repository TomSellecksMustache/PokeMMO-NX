#pragma once
#include "linux_visual_effects.h"

// All settings use controller buttons; each visible effect may be enabled separately.
#define LINUX_VISUAL_TABS 3u
static inline unsigned linuxVisualMenuRows(unsigned tab) { return tab < 2u ? 6u : (tab == 2u ? 3u : 0u); }
static inline unsigned linuxVisualMenuEffect(unsigned tab, unsigned row) {
    static const unsigned map[3][6] = {
        {VISUAL_COLOR_GRADE, VISUAL_VIGNETTE, VISUAL_VIBRANCE, VISUAL_CONTRAST, VISUAL_SHADOWS, VISUAL_HIGHLIGHTS},
        {VISUAL_SOFT_PIXELS, VISUAL_TILT_SHIFT, VISUAL_BLOOM, VISUAL_FXAA, VISUAL_SHARPEN, VISUAL_HAZE},
        {VISUAL_WIDE_GLOW, VISUAL_CINEMATIC_FOCUS, VISUAL_PAINTERLY}
    };
    return tab < LINUX_VISUAL_TABS && row < linuxVisualMenuRows(tab) ? map[tab][row] : VISUAL_EFFECT_COUNT;
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
        case VISUAL_WIDE_GLOW: return "Bright highlights spread over a wider area.";
        case VISUAL_CINEMATIC_FOCUS: return "Stronger graduated focus at the top and bottom.";
        case VISUAL_PAINTERLY: return "Smooths nearby colours while keeping edges distinct.";
        default: return "";
    }
}
