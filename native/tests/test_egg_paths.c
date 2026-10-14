#include "nxpanded_egg_paths.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    const char *filename="/tmp/nxpanded-egg-verified-test.tsv";
    FILE *f=fopen(filename,"wb");assert(f);
    /* Test fixture move names are deliberately not asserted to exist in game. */
    fputs("Gible\tTest Move\tGabite\tinherited\tverified\n",f);
    fputs("Gabite\tTest Move\tGarchomp\tknown\tverified\n",f);
    fputs("Gible\tTest Move\tEevee\tknown\tverified\n",f); /* incompatible egg groups */
    fputs("Gible\tTest Move\tDitto\tknown\tunverified\n",f);
    fclose(f);
    NxEggRoutes routes;assert(nxEggRoutesLoad(filename,&routes));
    assert(routes.count==2 && routes.rejected==2);
    NxEggPath path;
    BreedState stock={0};stock.stock_count=1;strcpy(stock.stock[0].name,"Garchomp");
    assert(nxEggFindPath(&routes,&stock,"Gible","Test Move",&path));
    assert(path.hops==2 && path.owned_source);
    assert(!strcmp(path.species[2],"Garchomp"));
    assert(!nxEggFindPath(&routes,&stock,"Gible","Unknown Move",&path));
    remove(filename);
    puts("NXpanded verified egg route tests: passed");
    return 0;
}
