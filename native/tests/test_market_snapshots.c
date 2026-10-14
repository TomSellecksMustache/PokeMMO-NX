#include "nxpanded_market.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    const char *filename="/tmp/nxpanded-market-test.tsv";
    FILE *f=fopen(filename,"wb");assert(f);
    fputs("#nx-market-v1\tUser-entered GTL notes\t1791970000\n",f);
    fputs("item\t0\t500\n",f);
    fputs("breeder\tGible\t1\t7000\n",f);
    fputs("breeder\t*\t3\t6000\n",f);
    fputs("breeder\tGabite\t1\t10\n",f);
    fputs("item\t99\t1\n",f);
    fclose(f);
    NxMarketSnapshot snap;
    assert(nxMarketLoad(filename,&snap));assert(snap.count==4 && snap.rejected==1);
    BreedState state={0};strcpy(state.species,"Gible");
    BreedPlan plan={0};plan.count=1;plan.need[0]=2;
    plan.steps[0].parent[0].kind=0;plan.steps[0].parent[0].ref=0;
    plan.steps[0].parent[1].kind=1;
    NxCosts fallback={.prices={100,0,0,0,0,0,0,9000}};
    NxMarketEstimate est;
    nxMarketEstimatePlan(&state,&plan,&fallback,&snap,1791970100u,&est);
    assert(est.known_total==1000+6000);
    assert(est.unknown_units==0 && est.matched_units==3);
    nxMarketEstimatePlan(&state,&plan,&fallback,&snap,1791970000u+604801u,&est);
    assert(est.stale && est.known_total==200+9000 && est.matched_units==0);
    remove(filename);
    puts("NXpanded market snapshot tests: passed");
    return 0;
}
