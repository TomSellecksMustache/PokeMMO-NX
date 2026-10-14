#pragma once
#include <stdbool.h>
#include <stdint.h>

// Applies only to the final OpenGL presentation image, never to game assets,
// client memory, or gameplay state. Individual effects can be combined.
typedef enum {
    VISUAL_TILT_SHIFT,
    VISUAL_BLOOM,
    VISUAL_COLOR_GRADE,
    VISUAL_SOFT_PIXELS,
    VISUAL_VIGNETTE,
    VISUAL_FXAA,
    VISUAL_VIBRANCE,
    VISUAL_CONTRAST,
    VISUAL_HIGHLIGHTS,
    VISUAL_SHARPEN,
    VISUAL_SHADOWS,
    VISUAL_HAZE,
    VISUAL_WIDE_GLOW,
    VISUAL_CINEMATIC_FOCUS,
    VISUAL_PAINTERLY,
    VISUAL_EFFECT_COUNT
} LinuxVisualEffect;

const char *linuxVisualEffectName(unsigned effect);
uint32_t linuxVisualEffectMask(void);
bool linuxVisualEffectEnabled(unsigned effect);
// Returns false if the setting works for this session but could not be saved.
bool linuxVisualEffectToggle(unsigned effect);
void linuxVisualEffectsPulse(void); // presentation state; does not persist
bool linuxVisualEffectsPulseActive(void);

// Portable settings helpers, also used by host-side tests.
bool linuxVisualSettingsRead(const char *path, uint32_t *mask);
bool linuxVisualSettingsWrite(const char *path, uint32_t mask);

// Runs on the GL render thread, before the cursor and NXpanded overlays.
// Failures leave the game's original framebuffer unchanged.
void linuxVisualEffectsRender(unsigned width, unsigned height);
void linuxVisualEffectsReset(void); // after EGL context destruction / before a new one
// 0: not initialized (or off), 1: shader ready, -1: failed and bypassed
int linuxVisualEffectsStatus(void);
