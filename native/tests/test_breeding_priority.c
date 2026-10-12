// Host-side regression tests: cc -std=c11 -I native/include native/source/breeding.c native/tests/test_breeding_priority.c -o /tmp/breed-test
#include "breeding.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stockReady(const BreedStep *step) {
    return step->parent[0].kind == 1 && step->parent[1].kind == 1 && step->have[0] && step->have[1];
}

static void validate(const BreedState *state, const BreedPlan *plan) {
    assert(plan->count <= BREED_STEPS);
    assert(plan->extra >= 0 && plan->extra <= 3);
    bool used_stock[BREED_STOCK] = {false};
    int allocated[BREED_ITEMS] = {0};
    int required[BREED_ITEMS] = {0};
    bool later_stock_ready = false;
    for (unsigned i = 0; i < plan->count; ++i) {
        const BreedStep *step = &plan->steps[i];
        assert(step->count <= BREED_STATS);
        if (i > 0 && stockReady(step)) later_stock_ready = true;
        for (unsigned j = 0; j < 2; ++j) {
            BreedParent p = step->parent[j];
            assert(p.kind >= 0 && p.kind <= 2);
            if (p.kind == 1) {
                assert(p.ref >= 0 && (unsigned)p.ref < state->stock_count);
                assert(!used_stock[p.ref]); // never consume the same Pokemon twice
                used_stock[p.ref] = true;
            } else if (p.kind == 2) {
                assert(p.ref >= 0 && (unsigned)p.ref < i); // parents must already exist
            } else {
                assert(p.ref == -1 || (unsigned)p.ref < BREED_STATS);
            }
            assert(step->item[j] >= 0 && (unsigned)step->item[j] < BREED_ITEMS);
            int item = step->item[j];
            required[item]++;
            if (step->have[j]) allocated[item]++;
        }
    }
    for (unsigned item = 0; item < BREED_ITEMS; ++item) {
        assert(required[item] == plan->need[item]);
        assert(allocated[item] <= state->have[item]);
    }
    // A step needing stock purchases must not precede another fully stocked step.
    if (later_stock_ready) assert(stockReady(&plan->steps[0]));
}

static void testStockFirstAndConsumption(void) {
    BreedState s = {0};
    BreedPlan p;
    strcpy(s.species, "Gible");
    s.nature = -1;
    s.ivs = 15; // HP, Atk, Def, SpA
    s.have[0] = 2;
    s.have[1] = 2;
    s.have[3] = 2;
    unsigned ivs[] = {5, 5, 1, 5, 6, 4, 1, 1, 10, 6};
    s.stock_count = sizeof(ivs) / sizeof(ivs[0]);
    for (unsigned i = 0; i < s.stock_count; ++i) {
        snprintf(s.stock[i].name, sizeof(s.stock[i].name), "P%u", i);
        s.stock[i].ivs = ivs[i];
        s.stock[i].nature = -1;
    }
    breedPlan(&s, &p);
    validate(&s, &p);
    assert(p.count == 4);
    // The original planner queued the missing-Power-Belt step before this one.
    assert(stockReady(&p.steps[0]));
    BreedState prior = s;
    const BreedStep first = p.steps[0];
    breedDone(&s, &p);
    assert(s.stock_count == prior.stock_count - 1); // two parent Pokemon consumed; one offspring added
    for (unsigned item = 0; item < BREED_ITEMS; ++item) {
        int consumed = 0;
        for (unsigned parent = 0; parent < 2; ++parent)
            if (first.have[parent] && first.item[parent] == (int)item) consumed++;
        assert(s.have[item] == prior.have[item] - consumed);
    }
    BreedPlan updated;
    breedPlan(&s, &updated);
    validate(&s, &updated);
    assert(updated.count <= p.count - 1);
}

static unsigned random_seed = 0x5eeda11u;
static unsigned nextRandom(void) { return random_seed = random_seed * 1664525u + 1013904223u; }

static void testRandomizedPlans(void) {
    for (int trial = 0; trial < 260; ++trial) {
        BreedState s = {0};
        BreedPlan p;
        strcpy(s.species, "Gible");
        s.nature = (nextRandom() % 3) == 0 ? 3 : -1;
        unsigned count = 2 + nextRandom() % 5;
        s.ivs = (1u << count) - 1u;
        s.stock_count = nextRandom() % 16;
        for (unsigned i = 0; i < s.stock_count; ++i) {
            strcpy(s.stock[i].name, "Owned");
            s.stock[i].ivs = nextRandom() & 63u;
            s.stock[i].nature = (nextRandom() & 3u) == 0 ? 3 : -1;
        }
        for (unsigned i = 0; i < BREED_ITEMS; ++i) s.have[i] = (int)(nextRandom() % 4);
        breedPlan(&s, &p);
        validate(&s, &p);
        if (p.count) {
            breedDone(&s, &p);
            breedPlan(&s, &p);
            validate(&s, &p);
        }
    }
}

static void testShoppingList(void) {
    BreedState s = {0};
    BreedPlan p = {0};
    int to_buy[BREED_ITEMS];
    p.need[0] = 5;
    p.need[1] = 2;
    p.need[6] = 4;
    s.have[0] = 2;
    s.have[1] = 99;
    s.have[6] = 3;
    breedShoppingList(&s, &p, to_buy);
    assert(to_buy[0] == 3);
    assert(to_buy[1] == 0);
    assert(to_buy[6] == 1);
    for (int i = 2; i < 6; ++i) assert(to_buy[i] == 0);
    s.have[6] = -5; // bad saved stock cannot generate a negative purchase count
    breedShoppingList(&s, &p, to_buy);
    assert(to_buy[6] == 4);
    memset(&p, 0, sizeof(p));
    breedShoppingList(&s, &p, to_buy);
    for (unsigned i = 0; i < BREED_ITEMS; ++i) assert(to_buy[i] == 0);
}
static void testBoundaries(void) {
    BreedState s = {0};
    BreedPlan p;
    strcpy(s.species, "Gible");
    s.nature = 3;
    s.ivs = 63;
    breedPlan(&s, &p);
    assert(p.count == 63); // 6x31 + nature is the maximum dependency tree
    validate(&s, &p);
    // One owned Pokemon can satisfy the entire target without new breeds.
    s.stock_count = 1;
    s.stock[0].ivs = 63;
    s.stock[0].nature = 3;
    strcpy(s.stock[0].name, "Perfect");
    breedPlan(&s, &p);
    assert(p.count == 0);
    validate(&s, &p);
}

int main(void) {
    testStockFirstAndConsumption();
    testRandomizedPlans();
    testBoundaries();
    testShoppingList();
    puts("Breeding inventory-priority tests passed.");
    return 0;
}
