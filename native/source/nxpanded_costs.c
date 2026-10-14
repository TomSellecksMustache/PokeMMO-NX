#include "nxpanded_costs.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static const char *const keys[NX_COST_COUNT] = {
    "hp", "attack", "defense", "sp_attack", "sp_defense", "speed", "everstone", "breeder"
};
const char *nxCostKey(unsigned index) { return index < NX_COST_COUNT ? keys[index] : ""; }
uint32_t nxCostAdjust(uint32_t value, int direction, uint32_t increment) {
    const uint32_t limit = 100000000u;
    if (!increment || !direction) return value;
    if (direction < 0) return value > increment ? value - increment : 0;
    return value > limit - increment ? limit : value + increment;
}
void nxCostEstimate(const BreedState *state, const BreedPlan *plan, const NxCosts *prices, NxCostEstimate *out) {
    memset(out, 0, sizeof(*out));
    if (!state || !plan || !prices) return;
    breedShoppingList(state, plan, out->outstanding);
    for (unsigned i = 0; i < BREED_ITEMS; ++i) {
        int count = out->outstanding[i];
        if (count <= 0) continue;
        if (!prices->prices[i]) out->unknown_units += (unsigned)count;
        else out->known_total += (uint64_t)count * prices->prices[i];
    }
    // Only count actual externally sourced parents, never offspring.
    // A reusable owned breeder does not incur the optional external price.
    unsigned external = 0;
    for (unsigned step = 0; step < plan->count; ++step)
        for (unsigned parent = 0; parent < 2; ++parent)
            external += plan->steps[step].parent[parent].kind == 0;
    out->breeders_to_get = external;
    if (!prices->prices[BREED_ITEMS]) out->unknown_units += external;
    else out->known_total += (uint64_t)external * prices->prices[BREED_ITEMS];
}
bool nxCostRead(const char *path, NxCosts *prices) {
    if (!path || !prices) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    NxCosts parsed = {{0}};
    char buf[160];
    while (fgets(buf, sizeof(buf), f)) {
        char key[48], trailer;
        unsigned long long price = 0;
        if (sscanf(buf, " %47[a-z_] = %llu %c", key, &price, &trailer) != 2 || price > 100000000u) continue;
        for (unsigned i = 0; i < NX_COST_COUNT; ++i)
            if (!strcmp(key, keys[i])) { parsed.prices[i] = (uint32_t)price; break; }
    }
    bool ok = !ferror(f);
    fclose(f);
    if (ok) *prices = parsed;
    return ok;
}
bool nxCostWrite(const char *path, const NxCosts *prices) {
    if (!path || !prices) return false;
    char tmp[256], bak[256];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp) ||
        snprintf(bak, sizeof(bak), "%s.bak", path) >= (int)sizeof(bak)) return false;
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = true;
    for (unsigned i = 0; i < NX_COST_COUNT; ++i)
        if (fprintf(f, "%s=%u\n", keys[i], prices->prices[i]) < 0) ok = false;
    if (fflush(f)) ok = false;
    if (fclose(f)) ok = false;
    if (!ok) { remove(tmp); return false; }
    FILE *old = fopen(path, "rb");
    bool existed = old != NULL;
    if (old) fclose(old);
    if (existed) {
        remove(bak);
        if (rename(path, bak)) { remove(tmp); return false; }
    }
    if (rename(tmp, path)) {
        if (existed) rename(bak, path);
        remove(tmp);
        return false;
    }
    if (existed) remove(bak);
    return true;
}
