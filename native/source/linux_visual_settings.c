#include "linux_visual_effects.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define SETTINGS_PATH "sdmc:/switch/PokeMMO-NXpanded/visual-effects.cfg"
#define EFFECT_MASK ((1u << VISUAL_EFFECT_COUNT) - 1u)

static const char *const names[VISUAL_EFFECT_COUNT] = {
    "Tilt-shift blur", "Soft bloom", "Colour grading", "Soften pixels", "Vignette",
    "Edge smoothing", "Vibrance", "Contrast", "Highlight rolloff", "Edge sharpening",
    "Shadow depth", "Atmospheric haze"
};
static const char *const keys[VISUAL_EFFECT_COUNT] = {
    "tilt_shift", "bloom", "color_grading", "soft_pixels", "vignette",
    "fxaa", "vibrance", "contrast", "highlights", "sharpen", "shadows", "haze"
};
static atomic_uint enabled_mask;
static atomic_bool loaded;
static atomic_flag settings_guard = ATOMIC_FLAG_INIT;

const char *linuxVisualEffectName(unsigned effect) {
    return effect < VISUAL_EFFECT_COUNT ? names[effect] : "Unknown";
}
// Unknown/malformed settings are ignored rather than allowing arbitrary masks.
bool linuxVisualSettingsRead(const char *path, uint32_t *mask) {
    if (!path || !mask) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    char line[128];
    uint32_t parsed = 0;
    while (fgets(line, sizeof(line), f)) {
        char key[64], value[16], extra;
        if (sscanf(line, " %63[^= ] = %15s %c", key, value, &extra) != 2) continue;
        for (unsigned i = 0; i < VISUAL_EFFECT_COUNT; ++i) {
            if (strcmp(key, keys[i])) continue;
            if (!strcmp(value, "1")) parsed |= 1u << i;
            else if (!strcmp(value, "0")) parsed &= ~(1u << i);
            break;
        }
    }
    bool ok = !ferror(f);
    fclose(f);
    if (ok) *mask = parsed & EFFECT_MASK;
    return ok;
}
bool linuxVisualSettingsWrite(const char *path, uint32_t mask) {
    if (!path) return false;
    char tmp[256], bak[256];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp) ||
        snprintf(bak, sizeof(bak), "%s.bak", path) >= (int)sizeof(bak)) return false;
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = true;
    for (unsigned i = 0; i < VISUAL_EFFECT_COUNT; ++i)
        if (fprintf(f, "%s=%d\n", keys[i], (mask >> i) & 1u) < 0) ok = false;
    if (fflush(f)) ok = false;
    if (fclose(f)) ok = false;
    if (!ok) { remove(tmp); return false; }
    // On Horizon OS rename() may not replace a destination. Preserve the old
    // configuration while replacing it, then restore it if installation fails.
    bool existed = false;
    FILE *old = fopen(path, "rb");
    if (old) { existed = true; fclose(old); }
    if (existed) {
        remove(bak);
        if (rename(path, bak) != 0) { remove(tmp); return false; }
    }
    if (rename(tmp, path) != 0) {
        if (existed) rename(bak, path);
        remove(tmp);
        return false;
    }
    if (existed) remove(bak);
    return true;
}
static atomic_bool extra_active;
void linuxVisualEffectsPulse(void) {
    atomic_store_explicit(&extra_active, !atomic_load_explicit(&extra_active, memory_order_relaxed), memory_order_release);
}
bool linuxVisualEffectsPulseActive(void) {
    return atomic_load_explicit(&extra_active, memory_order_acquire);
}
static void ensureLoaded(void) {
    if (atomic_load_explicit(&loaded, memory_order_acquire)) return;
    while (atomic_flag_test_and_set_explicit(&settings_guard, memory_order_acquire)) {}
    if (!atomic_load_explicit(&loaded, memory_order_relaxed)) {
        // Path exists in a normal NXpanded installation; make a best effort
        // when the app was copied without its data folder.
        mkdir("sdmc:/switch/PokeMMO-NXpanded", 0777);
        uint32_t mask = 0;
        linuxVisualSettingsRead(SETTINGS_PATH, &mask);
        atomic_store_explicit(&enabled_mask, mask, memory_order_release);
        atomic_store_explicit(&loaded, true, memory_order_release);
    }
    atomic_flag_clear_explicit(&settings_guard, memory_order_release);
}
uint32_t linuxVisualEffectMask(void) {
    ensureLoaded();
    return atomic_load_explicit(&enabled_mask, memory_order_acquire) & EFFECT_MASK;
}
bool linuxVisualEffectEnabled(unsigned effect) {
    return effect < VISUAL_EFFECT_COUNT && !!(linuxVisualEffectMask() & (1u << effect));
}
bool linuxVisualEffectToggle(unsigned effect) {
    if (effect >= VISUAL_EFFECT_COUNT) return false;
    ensureLoaded();
    while (atomic_flag_test_and_set_explicit(&settings_guard, memory_order_acquire)) {}
    uint32_t mask = atomic_load_explicit(&enabled_mask, memory_order_relaxed) ^ (1u << effect);
    atomic_store_explicit(&enabled_mask, mask, memory_order_release);
    bool saved = linuxVisualSettingsWrite(SETTINGS_PATH, mask);
    atomic_flag_clear_explicit(&settings_guard, memory_order_release);
    return saved;
}
