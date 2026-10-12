#include "nxpanded_text.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
    assert(nxTextMarqueeOffset(5, 20, 120, 5000000000ull) == 0);
    assert(nxTextMarqueeOffset(20, 20, 200, 0) == 0);
    assert(nxTextMarqueeOffset(20, 20, 200, 1000000000ull) == 0);
    float forward=nxTextMarqueeOffset(20,20,200,3000000000ull);
    assert(forward>60 && forward<80);
    assert(nxTextMarqueeOffset(20,20,200,10000000000ull)>forward);
    float end=nxTextMarqueeOffset(20,20,200,7500000000ull);
    assert(fabsf(end-200)<0.1);
    float reverse=nxTextMarqueeOffset(20,20,200,11500000000ull);
    assert(reverse < 200 && reverse > 50);
    assert(nxTextMarqueeOffset(20,20,200,14500000000ull)==0);
    assert(nxTextWrapLine("Short",20)==5);
    assert(nxTextWrapLine("One two three",7)==7);
    assert(nxTextWrapLine("Supercalifragilistic",7)==7);
    puts("marquee, clipping math, and word-wrap tests passed");
    return 0;
}
