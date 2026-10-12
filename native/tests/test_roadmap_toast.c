// Host-side tests for chord decoding, HUD expiry, and non-modal controller handling.
#include "linux_roadmap.h"
#include "linux_roadmap_shortcuts.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

// Console-only inline keyboard entry points. None may be called by the toast.
static int keyboard_requests;
bool linuxSdlInputKeyboardAvailable(void) { return true; }
bool linuxSdlInputKeyboardVisible(void) { return false; }
void linuxSdlInputKeyboardTarget(void (*target)(const char *, int)) { (void)target; }
void linuxSdlInputKeyboardRequest(bool show) { (void)show; ++keyboard_requests; }

// The host test does not need the database of egg groups or a real Switch SD card.
bool breedEggGroups(const char *species, char *out, size_t size) {
    (void)species;
    if (size) out[0] = 0;
    return false;
}
unsigned breedSpeciesCount(void) { return 0; }
const char *breedSpeciesName(unsigned index) { (void)index; return ""; }
bool breedSpeciesGroups(unsigned index, char *out, size_t size) {
    (void)index;
    if (size) out[0] = 0;
    return false;
}
bool breedSpeciesStarts(unsigned index, const char *typed) { (void)index; (void)typed; return false; }

static void shortcuts(void) {
    const uint32_t r = 1u << 10, r3 = 1u << 8, up = 1u << 11, left = 1u << 13, right = 1u << 14;
    assert(linuxRoadmapShortcut(r3) == ROADMAP_SHORTCUT_NONE); // keep the ordinary R3 keyboard
    assert(linuxRoadmapShortcut(r | up) == ROADMAP_SHORTCUT_NONE);
    assert(linuxRoadmapShortcut(r3 | up) == ROADMAP_SHORTCUT_NONE);
    assert(linuxRoadmapShortcut(r | r3 | right) == ROADMAP_SHORTCUT_MAIN);
    assert(linuxRoadmapShortcut(r | r3 | left) == ROADMAP_SHORTCUT_SHOPPING);
    assert(linuxRoadmapShortcut(r | r3 | up) == ROADMAP_SHORTCUT_TOAST);
    assert(linuxRoadmapShortcut(r | r3 | up | (1u << 0)) == ROADMAP_SHORTCUT_TOAST); // A unaffected
    assert(linuxRoadmapShortcut(r | r3 | up | left) == ROADMAP_SHORTCUT_SHOPPING);
    assert(linuxRoadmapShortcut(r | r3 | up | right) == ROADMAP_SHORTCUT_MAIN);
    assert(linuxRoadmapShortcut(r | r3 | (1u << 12)) == ROADMAP_SHORTCUT_NONE);
}
static void toast(void) {
    const uint64_t start = 1000000000ull;
    assert(linuxRoadmapToastOpacity(start) == 0.0f);
    assert(!linuxRoadmapCapturesInput());
    linuxRoadmapToastShow(start);
    assert(!linuxRoadmapCapturesInput()); // never stops the game
    assert(linuxRoadmapToastOpacity(start) == 1.0f);
    assert(fabsf(linuxRoadmapToastOpacity(start + 5000000000ull) - 0.5f) < 0.0001f);
    assert(linuxRoadmapToastOpacity(start + 9900000000ull) > 0.0f);
    assert(linuxRoadmapToastOpacity(start + 10000000000ull) == 0.0f);
    assert(linuxRoadmapToastOpacity(start + 20000000000ull) == 0.0f);
    assert(keyboard_requests == 0);
}
int main(void) {
    shortcuts();
    toast();
    puts("Breeding shortcut and toast tests passed.");
    return 0;
}
