#pragma once
#include "nxpanded_costs.h"
#include <stdbool.h>
#include <stdint.h>
#define NX_MARKET_MAX 128u
#define NX_MARKET_SOURCE_MAX 48

typedef struct { unsigned item_id; unsigned iv_mask; char species[40]; uint32_t price; } NxMarketPrice;
typedef struct {
    NxMarketPrice quotes[NX_MARKET_MAX]; unsigned count, rejected;
    char source[NX_MARKET_SOURCE_MAX]; uint64_t as_of_utc;
} NxMarketSnapshot;
typedef struct {
    NxCostEstimate base;
    uint64_t known_total;
    unsigned unknown_units, matched_units, covered_breeders;
    bool stale;
} NxMarketEstimate;
/* Offline TSV snapshot. Header: #nx-market-v1<TAB>source<TAB>unix_utc_timestamp
 * item<TAB>0..6<TAB>price, breeder<TAB>species<TAB>IV_mask_decimal<TAB>price.
 * No live GTL claims, no client access. */
bool nxMarketLoad(const char *path, NxMarketSnapshot *out);
void nxMarketEstimatePlan(const BreedState *state,const BreedPlan *plan,
                          const NxCosts *defaults,const NxMarketSnapshot *market,
                          uint64_t current_utc,NxMarketEstimate *out);
