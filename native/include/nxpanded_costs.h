#pragma once
#include "breeding.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Optional, user-entered estimates; zero means the price is unknown.
// Slot 7 is the cost of one unowned breeder (catch or purchase).
#define NX_COST_COUNT (BREED_ITEMS + 1u)
typedef struct { uint32_t prices[NX_COST_COUNT]; } NxCosts;
typedef struct {
    uint64_t known_total;
    unsigned unknown_units;
    unsigned breeders_to_get;
    int outstanding[BREED_ITEMS];
} NxCostEstimate;
void nxCostEstimate(const BreedState *state, const BreedPlan *plan, const NxCosts *costs, NxCostEstimate *out);
bool nxCostRead(const char *path, NxCosts *prices);
bool nxCostWrite(const char *path, const NxCosts *prices);
const char *nxCostKey(unsigned index);
uint32_t nxCostAdjust(uint32_t value, int direction, uint32_t increment);
