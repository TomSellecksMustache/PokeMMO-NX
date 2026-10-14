#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The breeding planner: from a target (species, nature, stats at 31) and what the player owns, the ordered breeds that make it. Plain C, no console calls.
#define BREED_STATS 6u
#define BREED_ITEMS 7u  // the six Power items (one per stat, in the order of the stats), then the Everstone
#define BREED_STOCK 64u
#define BREED_STEPS 64u
typedef struct {
    char name[40];
    unsigned ivs;  // a bit per stat that is 31
    int nature;    // an index of BREED_NATURES, -1: unknown
} BreedStock;
typedef struct {
    char species[24];
    int nature;  // -1: any
    unsigned ivs;
    int have[BREED_ITEMS];
    BreedStock stock[BREED_STOCK];
    unsigned stock_count;
    int left;  // breeds left when the chain was put aside, -1: not known
} BreedState;
typedef struct {
    int kind;  // 0: a Pokemon to catch or buy (ref: its stat, -1 for any), 1: one of the stock (ref: its index), 2: the offspring of an earlier step (ref: its index)
    int ref;
} BreedParent;
typedef struct {
    BreedParent parent[2];
    int item[2];  // held by each parent
    bool have[2];  // the player owns it
    unsigned char order[BREED_STATS], count;  // the stats of the offspring
    bool natured;
} BreedStep;
typedef struct {
    BreedStep steps[BREED_STEPS];
    unsigned count, leaves;
    int extra;  // breeds over the shortest route, to use what the player has
    int need[BREED_ITEMS];
} BreedPlan;

extern const char *const BREED_STAT_NAMES[BREED_STATS], *const BREED_ITEM_NAMES[BREED_ITEMS];
#define BREED_NATURE_COUNT 25u
extern const char *const BREED_NATURES[BREED_NATURE_COUNT];

const char *breedProblem(const BreedState *state);  // why there is nothing to plan, NULL when there is
void breedPlan(const BreedState *state, BreedPlan *plan);
// Price-aware planning among near-shortest legal IV trees. Zero = unknown, not free.
void breedPlanPriced(const BreedState *state, const uint32_t costs[BREED_ITEMS+1], BreedPlan *plan);
// For the entire remaining goal, not just the next step: quantities to buy after subtracting recorded stock.
void breedShoppingList(const BreedState *state, const BreedPlan *plan, int to_buy[BREED_ITEMS]);
void breedLabel(const BreedState *state, const BreedPlan *plan, BreedParent parent, char *out, size_t size);
void breedResult(const BreedState *state, const BreedStep *step, char *out, size_t size);  // "3x31 (Atk/Spe/HP) + Adamant"
void breedDone(BreedState *state, const BreedPlan *plan);                                   // the first step is made: its parents and items are used up, its offspring joins the stock
bool breedLoad(const char *path, BreedState *state);
bool breedSave(const char *path, const BreedState *state);
bool breedEggGroups(const char *species, char *out, size_t size);  // "Monster / Dragon"
unsigned breedSpeciesCount(void);                                    // the species the player can pick, in alphabetical order
const char *breedSpeciesName(unsigned index);
bool breedSpeciesGroups(unsigned index, char *out, size_t size);
bool breedSpeciesStarts(unsigned index, const char *typed);  // the name begins with what was typed (letters and digits only, any case)
