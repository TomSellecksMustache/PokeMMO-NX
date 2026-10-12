// Host-side tests for chord decoding, HUD expiry, and non-modal controller handling.
#include "linux_roadmap.h"
#include "linux_roadmap_shortcuts.h"
#include "linux_visual_menu.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// Console-only inline keyboard entry points. None may be called by the toast.
static int keyboard_requests;
// The actual renderer needs EGL; this host-side test checks only menu routing.
int linuxVisualEffectsStatus(void) { return 0; }
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
    const uint32_t r = 1u << 10, r3 = 1u << 8, up = 1u << 11, down = 1u << 12, left = 1u << 13, right = 1u << 14;
    assert(linuxRoadmapShortcut(r3) == ROADMAP_SHORTCUT_NONE); // keep the ordinary R3 keyboard
    assert(linuxRoadmapShortcut(r | up) == ROADMAP_SHORTCUT_NONE);
    assert(linuxRoadmapShortcut(r3 | up) == ROADMAP_SHORTCUT_NONE);
    assert(linuxRoadmapShortcut(r | r3 | right) == ROADMAP_SHORTCUT_MAIN);
    assert(linuxRoadmapShortcut(r | r3 | left) == ROADMAP_SHORTCUT_SHOPPING);
    assert(linuxRoadmapShortcut(r | r3 | up) == ROADMAP_SHORTCUT_TOAST);
    assert(linuxRoadmapShortcut(r | r3 | up | (1u << 0)) == ROADMAP_SHORTCUT_TOAST); // A unaffected
    assert(linuxRoadmapShortcut(r | r3 | up | left) == ROADMAP_SHORTCUT_SHOPPING);
    assert(linuxRoadmapShortcut(r | r3 | up | right) == ROADMAP_SHORTCUT_MAIN);
    assert(linuxRoadmapShortcut(r | r3 | down) == ROADMAP_SHORTCUT_VISUAL);
    assert(linuxRoadmapShortcut(r | r3 | down | (1u << 0)) == ROADMAP_SHORTCUT_VISUAL);
    assert(linuxRoadmapShortcut(r | r3 | up | down) == ROADMAP_SHORTCUT_TOAST);
    assert(linuxRoadmapShortcut(r | down) == ROADMAP_SHORTCUT_NONE);
    assert(linuxRoadmapShortcut(r3 | down) == ROADMAP_SHORTCUT_NONE);
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
// Simulate menu input using only the Switch controller, never touch.
static LinuxInputSnapshot menu_snapshot;
static bool seen_grade, seen_vignette, seen_soft, seen_tilt, seen_bloom, seen_title, seen_tab2;
static void fakeRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    (void)x; (void)y; (void)w; (void)h; (void)r; (void)g; (void)b; (void)a;
}
static void fakeText(float x, float y, const char *str, unsigned chars, float r, float g, float b) {
    (void)x; (void)y; (void)chars; (void)r; (void)g; (void)b;
    if (strstr(str, "NXpanded - Visual Enhancements")) seen_title = true;
    if (strstr(str, "Higher GPU cost")) seen_tab2 = true;
    if (strstr(str, "Colour grading") && strstr(str, "[ ")) seen_grade = true;
    if (strstr(str, "Vignette") && strstr(str, "[ ")) seen_vignette = true;
    if (strstr(str, "Soften pixels") && strstr(str, "[ ")) seen_soft = true;
    if (strstr(str, "Tilt-shift blur") && strstr(str, "[ ")) seen_tilt = true;
    if (strstr(str, "Soft bloom") && strstr(str, "[ ")) seen_bloom = true;
}
static void inspectTab(int expected_tab) {
    seen_grade = seen_vignette = seen_soft = seen_tilt = seen_bloom = seen_title = seen_tab2 = false;
    linuxRoadmapDraw(fakeRect, fakeText, NULL);
    assert(seen_title);
    assert(seen_tab2 == (expected_tab == 1));
    assert(seen_grade == (expected_tab == 0));
    assert(seen_vignette == (expected_tab == 0));
    assert(seen_soft == (expected_tab == 1));
    assert(seen_tilt == (expected_tab == 1));
    assert(seen_bloom == (expected_tab == 1));
}
static void menuPress(uint32_t buttons) {
    menu_snapshot.buttons = 0;
    menu_snapshot.timestamp_ns += 10000000ull;
    linuxRoadmapFeed(&menu_snapshot);
    menu_snapshot.buttons = buttons;
    menu_snapshot.timestamp_ns += 10000000ull;
    linuxRoadmapFeed(&menu_snapshot);
}
static void visualMenu(void) {
    assert(LINUX_VISUAL_TABS == 2);
    assert(linuxVisualMenuRows(0) == 2 && linuxVisualMenuRows(1) == 3);
    assert(linuxVisualMenuRows(2) == 0);
    assert(linuxVisualMenuEffect(0, 0) == VISUAL_COLOR_GRADE);
    assert(linuxVisualMenuEffect(0, 1) == VISUAL_VIGNETTE);
    assert(linuxVisualMenuEffect(1, 0) == VISUAL_SOFT_PIXELS);
    assert(linuxVisualMenuEffect(1, 1) == VISUAL_TILT_SHIFT);
    assert(linuxVisualMenuEffect(1, 2) == VISUAL_BLOOM);
    assert(linuxVisualMenuEffect(1, 3) == VISUAL_EFFECT_COUNT);
    assert(!linuxRoadmapCapturesInput());
    linuxRoadmapShowVisual();
    assert(linuxRoadmapOpen() && linuxRoadmapCapturesInput());
    menu_snapshot = (LinuxInputSnapshot){0};
    menu_snapshot.gamepad = true;
    menu_snapshot.timestamp_ns = 10000000000ull;
    linuxRoadmapFeed(&menu_snapshot); // ignore input held on opening
    inspectTab(0);
    // R changes to heavy effects; A changes only the currently selected effect.
    unsigned before_switch = linuxVisualEffectEnabled(VISUAL_SOFT_PIXELS);
    menuPress((1u << 10) | (1u << 0)); // tab switch + A must NOT toggle the first heavy effect
    inspectTab(1);
    assert(linuxVisualEffectEnabled(VISUAL_SOFT_PIXELS) == before_switch);
    unsigned old_soft = linuxVisualEffectEnabled(VISUAL_SOFT_PIXELS);
    unsigned old_grade = linuxVisualEffectEnabled(VISUAL_COLOR_GRADE);
    menuPress(1u << 0);
    assert(linuxVisualEffectEnabled(VISUAL_SOFT_PIXELS) != old_soft);
    assert(linuxVisualEffectEnabled(VISUAL_COLOR_GRADE) == old_grade);
    menuPress(1u << 12); // down to tilt-shift
    unsigned old_tilt = linuxVisualEffectEnabled(VISUAL_TILT_SHIFT);
    menuPress(1u << 0);
    assert(linuxVisualEffectEnabled(VISUAL_TILT_SHIFT) != old_tilt);
    // L wraps back to the light tab and resets selection to the first item.
    menuPress(1u << 9);
    inspectTab(0);
    menuPress(1u << 11); // up at first item stays on the first item
    menuPress(1u << 0);
    assert(linuxVisualEffectEnabled(VISUAL_COLOR_GRADE) != old_grade);
    // L wraps backwards, too. B closes even if A is simultaneously pressed.
    menuPress(1u << 9);
    inspectTab(1);
    unsigned old_bloom = linuxVisualEffectEnabled(VISUAL_BLOOM);
    menuPress((1u << 0) | (1u << 1));
    assert(!linuxRoadmapOpen());
    assert(linuxVisualEffectEnabled(VISUAL_BLOOM) == old_bloom);
    assert(linuxRoadmapCapturesInput()); // hold closing button until release
    menu_snapshot.buttons = 0;
    menu_snapshot.timestamp_ns += 10000000ull;
    linuxRoadmapFeed(&menu_snapshot);
    assert(!linuxRoadmapCapturesInput());
    assert(keyboard_requests == 0);
}
int main(void) {
    shortcuts();
    toast();
    visualMenu();
    puts("Breeding shortcut, toast, and controller-tab tests passed.");
    return 0;
}
