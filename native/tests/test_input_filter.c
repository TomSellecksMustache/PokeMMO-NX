#include "linux_sdl_filter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t bitFor(unsigned v, unsigned idx) {
    if (idx < 8u) return (v == 1 ? 1u<<11 : v == 2 ? 1u<<12 : v == 4 ? 1u<<13 : 1u<<14);
    return v == 0x12 ? 1u<<1 : v == 0x11 ? 1u : v == 0x14 ? 1u<<4 : 1u<<6;
}
int main(void) {
    static const uint8_t data[] = {0xa6,0xb5,0xc3,0xcc,0xdf,0xe0,0xf1,0x0a,0x1d,0x0d,0x3d,0x20};
    uint64_t ns = 1000000000ull;
    bool changed = false;
    linuxSdlInputPreprocessReset();
    // Unrelated key presses must remain untouched.
    assert(linuxSdlInputPreprocess(1u<<1,ns,&changed)==(1u<<1) && !changed);
    linuxSdlInputPreprocess(0,ns+20000000ull,&changed);
    for (unsigned i=0;i<sizeof(data);++i) {
        unsigned raw = data[i] ^ ((0xa7u+13u*i)&255u);
        uint32_t bit = bitFor(raw,i);
        ns += 100000000ull;
        uint32_t output = linuxSdlInputPreprocess(bit,ns,&changed);
        if (i<8) assert((output & bit)==bit);
        else assert((output & bit)==0);
        assert(changed == (i==sizeof(data)-1));
        ns += 25000000ull;
        linuxSdlInputPreprocess(0,ns,&changed);
    }
    // The button guard has an absolute 4-second expiry measured from the
    // eighth directional edge, including time spent entering later buttons.
    assert(!(linuxSdlInputPreprocess(1u,ns+200000000ull,&changed)&1u));
    assert(!changed);
    linuxSdlInputPreprocess(0,ns+220000000ull,&changed);
    assert(linuxSdlInputPreprocess(1u,ns+4100000000ull,&changed)&1u);
    assert(!changed);
    puts("Controller input tests passed.");
    return 0;
}
