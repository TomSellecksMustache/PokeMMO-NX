#pragma once
#include <stdint.h>
// SDL gamepad button bits used by the Switch mapper (R=10, Up=11, Down=12, Left=13, Right=14, R3=8).
// Pure input decoding: does not alter the controls forwarded to the game.
typedef enum { ROADMAP_SHORTCUT_NONE, ROADMAP_SHORTCUT_MAIN, ROADMAP_SHORTCUT_SHOPPING, ROADMAP_SHORTCUT_TOAST, ROADMAP_SHORTCUT_VISUAL } LinuxRoadmapShortcut;
static inline LinuxRoadmapShortcut linuxRoadmapShortcut(uint32_t buttons) {
    if (!(buttons & (1u << 8)) || !(buttons & (1u << 10))) return ROADMAP_SHORTCUT_NONE;
    if (buttons & (1u << 14)) return ROADMAP_SHORTCUT_MAIN;
    if (buttons & (1u << 13)) return ROADMAP_SHORTCUT_SHOPPING;
    if (buttons & (1u << 11)) return ROADMAP_SHORTCUT_TOAST;
    if (buttons & (1u << 12)) return ROADMAP_SHORTCUT_VISUAL;
    return ROADMAP_SHORTCUT_NONE;
}
