#include "nxpanded_costs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    BreedState state = {0};
    BreedPlan plan = {0};
    NxCosts price = { .prices = {100,200,300,400,500,600,700,2000} };
    plan.count = 2;
    plan.need[0] = 2;
    plan.need[BREED_ITEMS-1] = 1;
    state.have[0] = 1;
    plan.steps[0].parent[0].kind = 0;
    plan.steps[0].parent[1].kind = 1;
    plan.steps[1].parent[0].kind = 2;
    plan.steps[1].parent[1].kind = 0;
    NxCostEstimate est;
    nxCostEstimate(&state,&plan,&price,&est);
    assert(est.outstanding[0]==1);
    assert(est.outstanding[BREED_ITEMS-1]==1);
    assert(est.breeders_to_get==2);
    assert(est.known_total==100+700+2*2000);
    assert(est.unknown_units==0);
    price.prices[0]=0;
    nxCostEstimate(&state,&plan,&price,&est);
    assert(est.known_total==700+2*2000 && est.unknown_units==1);
    assert(nxCostAdjust(0,-1,1000)==0);
    assert(nxCostAdjust(123,1,1000)==1123);
    assert(nxCostAdjust(100000000,1,1000)==100000000);
    const char *path="/tmp/nxpanded-cost-test.cfg";
    remove(path);
    assert(nxCostWrite(path,&price));
    NxCosts loaded={0};
    assert(nxCostRead(path,&loaded));
    assert(memcmp(&loaded,&price,sizeof(price))==0);
    loaded.prices[0]=1300;
    assert(nxCostWrite(path,&loaded));
    NxCosts updated={0};
    assert(nxCostRead(path,&updated));
    assert(updated.prices[0]==1300);
    remove(path);
    puts("NXpanded cost estimator: passed");
    return 0;
}
