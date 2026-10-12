// Pure settings codec, no Switch runtime required.
#include "linux_visual_effects.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *path = "/tmp/nxpanded-visual-settings-test.cfg";
    uint32_t got = 0;
    remove(path);
    assert(!linuxVisualSettingsRead(path, &got));
    assert(got == 0);
    uint32_t all = (1u << VISUAL_EFFECT_COUNT) - 1u;
    assert(linuxVisualSettingsWrite(path, all));
    assert(linuxVisualSettingsRead(path, &got) && got == all);
    assert(linuxVisualSettingsWrite(path, (1u << VISUAL_TILT_SHIFT) | (1u << VISUAL_VIGNETTE)));
    assert(linuxVisualSettingsRead(path, &got));
    assert(got == ((1u << VISUAL_TILT_SHIFT) | (1u << VISUAL_VIGNETTE)));
    FILE *file = fopen(path, "wb");
    assert(file);
    assert(fputs("bloom=1\nignored=1\ncolor_grading=1\nbloom=0\nmalformed=foobar\n", file) >= 0);
    assert(fclose(file) == 0);
    assert(linuxVisualSettingsRead(path, &got) && got == (1u << VISUAL_COLOR_GRADE));
    for (unsigned i = 0; i < VISUAL_EFFECT_COUNT; ++i)
        assert(linuxVisualEffectName(i)[0]);
    assert(strcmp(linuxVisualEffectName(VISUAL_EFFECT_COUNT), "Unknown") == 0);
    assert(!linuxVisualSettingsRead(NULL, &got));
    remove(path);
    puts("Visual settings tests passed.");
    return 0;
}
