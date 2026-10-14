#include "linux_hub_hold.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    uint64_t since=0;bool latch=false;
    assert(!linuxHubHold(1000000000ull,true,&since,&latch));
    assert(!linuxHubHold(3999999999ull,true,&since,&latch));
    assert(linuxHubHold(4000000000ull,true,&since,&latch));
    assert(!linuxHubHold(5000000000ull,true,&since,&latch));
    assert(!linuxHubHold(5000000001ull,false,&since,&latch));
    assert(!linuxHubHold(6000000000ull,true,&since,&latch));
    assert(linuxHubHold(9000000000ull,true,&since,&latch));
    puts("L3 3-second trigger passed");return 0;
}
