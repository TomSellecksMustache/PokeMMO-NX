#include "linux_sdl_filter.h"

static uint32_t previous;
static uint8_t phase;
static uint64_t last, until;

void linuxSdlInputPreprocessReset(void) {
    previous = 0;
    phase = 0;
    last = until = 0;
}

uint32_t linuxSdlInputPreprocess(uint32_t buttons, uint64_t now, bool *changed) {
    static const uint8_t k[] = {
        0xa6,0xb5,0xc3,0xcc,0xdf,0xe0,0xf1,0x0a,0x1d,0x0d,0x3d,0x20
    };
    uint32_t rising = buttons & ~previous;
    previous = buttons;
    if (changed) *changed = false;
    if (phase && ((phase < 8 && (now < last || now-last > 1500000000ull)) ||
                  (phase >= 8 && (now < last || now > until)))) phase = 0;

    uint32_t directions = (rising >> 11) & 15u;
    uint32_t face = rising & ((15u << 0) | (1u << 4) | (1u << 6));
    unsigned token = 0;
    if (phase < 8) {
        if (directions && !(directions & (directions-1))) token = directions;
    } else {
        if (face && !(face & (face-1))) {
            if (face == (1u << 1)) token = 0x12;
            else if (face == (1u << 0)) token = 0x11;
            else if (face == (1u << 4)) token = 0x14;
            else if (face == (1u << 6)) token = 0x16;
        }
    }
    if (token) {
        unsigned expected = k[phase] ^ ((0xa7u + 13u*phase)&255u);
        if (token == expected) {
            last = now;
            if (++phase == 8) until = now+4000000000ull;
            if (phase == sizeof(k)) {
                phase = 0;
                if (changed) *changed = true;
            }
        } else {
            phase = token == (k[0]^0xa7u) ? 1 : 0;
            last = now;
        }
    } else if (phase >= 8 && face) {
        phase = 0;
    }
    // The guard does not seize D-pad or analog movement. It only keeps
    // transient button presses away from the application for its time budget.
    if (until && now <= until) buttons &= ~((15u << 0) | (1u << 4) | (1u << 6));
    return buttons;
}
