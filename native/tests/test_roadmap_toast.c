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
static bool seen_grade, seen_vignette, seen_soft, seen_tilt, seen_bloom, seen_title, seen_tab2, seen_tab3, seen_wide, seen_focus, seen_painter;
static void fakeRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    (void)x; (void)y; (void)w; (void)h; (void)r; (void)g; (void)b; (void)a;
}
static void fakeText(float x, float y, const char *str, unsigned chars, float r, float g, float b) {
    (void)x; (void)y; (void)chars; (void)r; (void)g; (void)b;
    if (strstr(str, "NXpanded - Visual Enhancements")) seen_title = true;
    if (strstr(str, "Six optional image effects")) seen_tab2 = true;
    if (strstr(str, "Three experimental, GPU-heavy filters")) seen_tab3 = true;
    if (strstr(str, "Wide cinematic glow") && strstr(str, "[ ")) seen_wide = true;
    if (strstr(str, "Cinematic focus") && strstr(str, "[ ")) seen_focus = true;
    if (strstr(str, "Painterly smoothing") && strstr(str, "[ ")) seen_painter = true;
    if (strstr(str, "Colour grading") && strstr(str, "[ ")) seen_grade = true;
    if (strstr(str, "Vignette") && strstr(str, "[ ")) seen_vignette = true;
    if (strstr(str, "Soften pixels") && strstr(str, "[ ")) seen_soft = true;
    if (strstr(str, "Tilt-shift blur") && strstr(str, "[ ")) seen_tilt = true;
    if (strstr(str, "Soft bloom") && strstr(str, "[ ")) seen_bloom = true;
}
static void inspectTab(int expected_tab) {
    seen_grade = seen_vignette = seen_soft = seen_tilt = seen_bloom = seen_title = seen_tab2 = seen_tab3 = seen_wide = seen_focus = seen_painter = false;
    linuxRoadmapDraw(fakeRect, fakeText, NULL);
    assert(seen_title);
    assert(seen_tab2 == (expected_tab == 1));
    assert(seen_tab3 == (expected_tab == 2));
    assert(seen_wide == (expected_tab == 2));
    assert(seen_focus == (expected_tab == 2));
    assert(seen_painter == (expected_tab == 2));
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
static bool saw_shop, saw_road;
static void tabText(float x, float y, const char *str, unsigned chars, float r, float g, float b) {
    (void)x; (void)y; (void)chars; (void)r; (void)g; (void)b;
    if (strstr(str, "Shopping list -")) saw_shop = true;
    if (strstr(str, "Breeding roadmap -") || strstr(str, "Next step -")) saw_road = true;
}
static float toast_first_x = -1, toast_first_w = -1;
static void toastRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    (void)y; (void)h; (void)r; (void)g; (void)b; (void)a;
    if (toast_first_x < 0) { toast_first_x = x; toast_first_w = w; }
}
static void unifiedRoadmap(void) {
    // The two pages are navigated with shoulder buttons, no touchscreen input.
    menu_snapshot = (LinuxInputSnapshot){0};
    menu_snapshot.gamepad = true;
    menu_snapshot.timestamp_ns = 12000000000ull;
    linuxRoadmapShowShopping();
    linuxRoadmapFeed(&menu_snapshot);
    saw_shop = saw_road = false;
    linuxRoadmapDraw(fakeRect, tabText, NULL);
    assert(saw_shop && !saw_road);
    menuPress(1u << 9); // L selects roadmap from shopping
    saw_shop = saw_road = false;
    linuxRoadmapDraw(fakeRect, tabText, NULL);
    assert(saw_road && !saw_shop);
    menuPress(1u << 10); // R selects shopping
    saw_shop = saw_road = false;
    linuxRoadmapDraw(fakeRect, tabText, NULL);
    assert(saw_shop && !saw_road);
    menuPress(1u << 1); // B closes whichever tab is active
    assert(!linuxRoadmapOpen());
    menu_snapshot.buttons = 0;
    linuxRoadmapFeed(&menu_snapshot);
    assert(!linuxRoadmapCapturesInput());
    linuxRoadmapToastShow(13000000000ull);
    linuxRoadmapToastDraw(toastRect, fakeText, NULL, 1.0f);
    assert(fabsf((toast_first_x + toast_first_w * 0.5f) - 640.0f) < 0.01f);
}
static void visualMenu(void) {
    assert(LINUX_VISUAL_TABS == 3);
    assert(linuxVisualMenuRows(0) == 6 && linuxVisualMenuRows(1) == 6);
    assert(linuxVisualMenuRows(2) == 3 && linuxVisualMenuRows(3) == 0);
    assert(linuxVisualMenuEffect(2, 0) == VISUAL_WIDE_GLOW);
    assert(linuxVisualMenuEffect(2, 1) == VISUAL_CINEMATIC_FOCUS);
    assert(linuxVisualMenuEffect(2, 2) == VISUAL_PAINTERLY);
    assert(linuxVisualMenuEffect(2, 3) == VISUAL_EFFECT_COUNT);
    assert(linuxVisualMenuEffect(0, 0) == VISUAL_COLOR_GRADE);
    assert(linuxVisualMenuEffect(0, 1) == VISUAL_VIGNETTE);
    assert(linuxVisualMenuEffect(1, 0) == VISUAL_SOFT_PIXELS);
    assert(linuxVisualMenuEffect(1, 1) == VISUAL_TILT_SHIFT);
    assert(linuxVisualMenuEffect(1, 2) == VISUAL_BLOOM);
    assert(linuxVisualMenuEffect(0, 2) == VISUAL_VIBRANCE);
    assert(linuxVisualMenuEffect(0, 3) == VISUAL_CONTRAST);
    assert(linuxVisualMenuEffect(0, 4) == VISUAL_SHADOWS);
    assert(linuxVisualMenuEffect(0, 5) == VISUAL_HIGHLIGHTS);
    assert(linuxVisualMenuEffect(1, 3) == VISUAL_FXAA);
    assert(linuxVisualMenuEffect(1, 4) == VISUAL_SHARPEN);
    assert(linuxVisualMenuEffect(1, 5) == VISUAL_HAZE);
    assert(linuxVisualMenuEffect(1, 6) == VISUAL_EFFECT_COUNT);
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
    // L cycles to the light tab, resetting selection to the first item.
    menuPress(1u << 9);
    inspectTab(0);
    menuPress(1u << 11); // up at first item stays on the first item
    menuPress(1u << 0);
    assert(linuxVisualEffectEnabled(VISUAL_COLOR_GRADE) != old_grade);
    // L wraps backwards to the new GPU-intensive third tab.
    menuPress(1u << 9);
    inspectTab(2);
    unsigned before_wide = linuxVisualEffectEnabled(VISUAL_WIDE_GLOW);
    menuPress(1u << 0);
    assert(linuxVisualEffectEnabled(VISUAL_WIDE_GLOW) != before_wide);
    menuPress(1u << 12);
    unsigned before_focus = linuxVisualEffectEnabled(VISUAL_CINEMATIC_FOCUS);
    menuPress(1u << 0);
    assert(linuxVisualEffectEnabled(VISUAL_CINEMATIC_FOCUS) != before_focus);
    menuPress(1u << 12);
    unsigned before_painter = linuxVisualEffectEnabled(VISUAL_PAINTERLY);
    menuPress(1u << 0);
    assert(linuxVisualEffectEnabled(VISUAL_PAINTERLY) != before_painter);
    // B closes even if A is simultaneously pressed.
    unsigned painter_at_close = linuxVisualEffectEnabled(VISUAL_PAINTERLY);
    menuPress((1u << 0) | (1u << 1));
    assert(!linuxRoadmapOpen());
    assert(linuxVisualEffectEnabled(VISUAL_PAINTERLY) == painter_at_close);
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
    unifiedRoadmap();
    puts("Breeding shortcut, toast, and controller-tab tests passed.");
    return 0;
}
