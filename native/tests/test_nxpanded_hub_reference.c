#include "nxpanded_hub_reference.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    const char *file="/tmp/nxpanded-hub-moves-test.tsv";
    FILE *f=fopen(file,"wb"); assert(f);
    fputs("# species\tmove\tdonor\nGible\tIron Head\tAron\nGIBLE\tOutrage\tDragonite\nPikachu\tFake Out\tSkitty\n",f);
    fclose(f);
    NxHubEggMove moves[4]={0};
    assert(nxHubEggMoves(file,"gible",moves,4)==2);
    assert(!strcmp(moves[0].move,"Iron Head"));
    assert(!strcmp(moves[1].donor,"Dragonite"));
    assert(nxHubEggMoves(file,"Ditto",moves,4)==0);
    remove(file);
    puts("NXpanded offline move reference: passed");
    return 0;
}
