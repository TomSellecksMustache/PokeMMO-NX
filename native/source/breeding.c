#include "breeding.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

const char *const BREED_STAT_NAMES[BREED_STATS] = {"HP", "Atk", "Def", "SpA", "SpD", "Spe"};
const char *const BREED_ITEM_NAMES[BREED_ITEMS] = {"Power Weight", "Power Bracer", "Power Belt", "Power Lens", "Power Band", "Power Anklet", "Everstone"};
const char *const BREED_NATURES[BREED_NATURE_COUNT] = {"Hardy",  "Lonely", "Brave",  "Adamant", "Naughty",  "Bold",   "Docile", "Relaxed", "Impish",
                                                       "Lax",    "Timid",  "Hasty",  "Serious", "Jolly",    "Naive",  "Modest", "Mild",    "Quiet",
                                                       "Bashful", "Rash",  "Calm",   "Gentle",  "Sassy",    "Careful", "Quirky"};
// Babies, legendaries, Ditto, Unown and Shedinja
static const char *const NO_BREED[] = {
    "ditto", "unown", "shedinja", "pichu", "riolu", "smoochum", "elekid", "magby",
    "togepi", "azurill", "wynaut", "budew", "chingling", "bonsly", "mime jr", "happiny",
    "munchlax", "mantyke", "tyrogue", "cleffa", "igglybuff", "articuno", "zapdos", "moltres",
    "mewtwo", "mew", "raikou", "entei", "suicune", "lugia", "ho-oh", "celebi",
    "regirock", "regice", "registeel", "latias", "latios", "kyogre", "groudon", "rayquaza",
    "jirachi", "deoxys", "uxie", "mesprit", "azelf", "dialga", "palkia", "heatran",
    "regigigas", "giratina", "cresselia", "darkrai", "shaymin", "arceus", "manaphy", "phione"};

typedef struct {
    const BreedState *state;
    BreedPlan *plan;
    bool used[BREED_STOCK];
} Build;
typedef struct {
    BreedPlan plan;
    int missing, total;
    int first_ready, first_stock, first_items, stock_used;
    uint64_t priced_cost;
    unsigned price_unknown;
} Eval;
typedef struct {
    const BreedState *state;
    const uint32_t *prices;
    Eval work, best;
    bool has_best;
    int pass, shortest;
} Search;

// A stock Pokemon that already has the stats (and the nature, when asked): the one with the fewest 31s, then without a nature, goes first.
static int takeStock(Build *build, const unsigned char *order, unsigned count, bool natured) {
    unsigned wanted = 0;
    for (unsigned i = 0; i < count; ++i) wanted |= 1u << order[i];
    int pick = -1;
    unsigned best = 0;
    for (unsigned i = 0; i < build->state->stock_count; ++i) {
        const BreedStock *mon = &build->state->stock[i];
        if (build->used[i] || (mon->ivs & wanted) != wanted || (natured && mon->nature != build->state->nature)) continue;
        unsigned key = (unsigned)__builtin_popcount(mon->ivs) * 2u + (mon->nature >= 0 ? 1u : 0u);
        if (pick < 0 || key < best) pick = (int)i, best = key;
    }
    if (pick >= 0) build->used[pick] = true;
    return pick;
}
static BreedParent addStep(Build *build, BreedParent first, int first_item, BreedParent second, int second_item, const unsigned char *order, unsigned count, bool natured) {
    BreedStep *step = &build->plan->steps[build->plan->count];
    step->parent[0] = first, step->parent[1] = second;
    step->item[0] = first_item, step->item[1] = second_item;
    memcpy(step->order, order, count);
    step->count = (unsigned char)count;
    step->natured = natured;
    return (BreedParent){2, (int)build->plan->count++};
}
static BreedParent buildPlain(Build *build, const unsigned char *order, unsigned count) {
    int stock = takeStock(build, order, count, false);
    if (stock >= 0) return (BreedParent){1, stock};
    if (count <= 1) {
        build->plan->leaves++;
        return (BreedParent){0, count ? order[0] : -1};
    }
    unsigned char without_b[BREED_STATS], without_a[BREED_STATS];
    unsigned na = 0, nb = 0;
    int a = order[count - 1], b = order[count - 2];
    for (unsigned i = 0; i < count; ++i) {
        if (order[i] != b) without_b[na++] = order[i];
        if (order[i] != a) without_a[nb++] = order[i];
    }
    BreedParent first = buildPlain(build, without_b, na);
    BreedParent second = buildPlain(build, without_a, nb);
    return addStep(build, first, a, second, b, order, count, false);
}
static BreedParent buildNatured(Build *build, const unsigned char *order, unsigned count) {
    int stock = takeStock(build, order, count, true);
    if (stock >= 0) return (BreedParent){1, stock};
    if (!count) {
        build->plan->leaves++;
        return (BreedParent){0, -1};
    }
    BreedParent first = buildNatured(build, order, count - 1);
    BreedParent second = buildPlain(build, order, count);
    return addStep(build, first, (int)BREED_ITEMS - 1, second, order[count - 1], order, count, true);
}
// Reorder independent breeds around the inventory, not just the recursive build order.
// Only steps whose offspring dependencies are already scheduled can move forward.
// Reserving each held item as a step is scheduled also makes the "have" flags
// describe the *new* queue instead of the original tree traversal.
static void prioritize(const BreedState *state, BreedPlan *plan) {
    BreedStep queued[BREED_STEPS];
    bool scheduled[BREED_STEPS] = {false};
    int new_index[BREED_STEPS];
    int items[BREED_ITEMS];
    memcpy(items, state->have, sizeof(items));

    for (unsigned pos = 0; pos < plan->count; ++pos) {
        int chosen = -1;
        int best_missing = 5, best_owned = -1, best_items = -1;
        for (unsigned i = 0; i < plan->count; ++i) {
            if (scheduled[i]) continue;
            const BreedStep *step = &plan->steps[i];
            bool blocked = false;
            int owned = 0, missing = 0;
            for (unsigned j = 0; j < 2; ++j) {
                if (step->parent[j].kind == 2 && !scheduled[step->parent[j].ref]) blocked = true;
                if (step->parent[j].kind == 1) owned++;
                if (step->parent[j].kind == 0) missing++;
            }
            if (blocked) continue;

            int remaining[BREED_ITEMS];
            memcpy(remaining, items, sizeof(remaining));
            int available = 0;
            for (unsigned j = 0; j < 2; ++j) {
                int item = step->item[j];
                if (remaining[item] > 0) remaining[item]--, available++;
            }
            missing += 2 - available;
            // A fully stocked breed comes first; among equal shortages prefer
            // parents already owned over ones that still need to be acquired.
            if (chosen < 0 || missing < best_missing ||
                (missing == best_missing && owned > best_owned) ||
                (missing == best_missing && owned == best_owned && available > best_items)) {
                chosen = (int)i;
                best_missing = missing;
                best_owned = owned;
                best_items = available;
            }
        }
        // The generated plan is a DAG; there is always an eligible step.
        if (chosen < 0) return;
        scheduled[chosen] = true;
        new_index[chosen] = (int)pos;
        BreedStep step = plan->steps[chosen];
        for (unsigned j = 0; j < 2; ++j) {
            if (step.parent[j].kind == 2) step.parent[j].ref = new_index[step.parent[j].ref];
            int item = step.item[j];
            step.have[j] = items[item] > 0;
            if (step.have[j]) items[item]--;
        }
        queued[pos] = step;
    }
    memcpy(plan->steps, queued, plan->count * sizeof(queued[0]));
}

static void evaluate(const BreedState *state, const unsigned char *order, unsigned count, Eval *eval) {
    Build build = {state, &eval->plan, {false}};
    memset(eval->plan.need, 0, sizeof(eval->plan.need));
    eval->plan.count = eval->plan.leaves = 0;
    if (state->nature >= 0)
        buildNatured(&build, order, count);
    else
        buildPlain(&build, order, count);
    prioritize(state, &eval->plan);

    eval->missing = eval->total = 0;
    eval->stock_used = 0;
    for (unsigned i = 0; i < eval->plan.count; ++i)
        for (unsigned j = 0; j < 2; ++j) {
            const BreedStep *step = &eval->plan.steps[i];
            eval->plan.need[step->item[j]]++;
            eval->total++;
            if (!step->have[j]) eval->missing++;
            if (step->parent[j].kind == 1) eval->stock_used++;
        }
    eval->priced_cost = 0;
    eval->price_unknown = 0;
    eval->first_ready = eval->first_stock = eval->first_items = 0;
    if (eval->plan.count) {
        const BreedStep *first = &eval->plan.steps[0];
        for (unsigned j = 0; j < 2; ++j) {
            if (first->parent[j].kind == 1) eval->first_stock++;
            if (first->have[j]) eval->first_items++;
        }
        eval->first_ready = eval->first_stock == 2 && eval->first_items == 2;
    }
}
static bool better(const Eval *a, const Eval *b, bool price_mode) {
    if (price_mode) {
        if (a->price_unknown != b->price_unknown)
            return a->price_unknown < b->price_unknown;
        if (a->priced_cost != b->priced_cost)
            return a->priced_cost < b->priced_cost;
    }
    if (a->missing != b->missing) return a->missing < b->missing;
    if (a->plan.count != b->plan.count) return a->plan.count < b->plan.count;
    if (a->first_ready != b->first_ready) return a->first_ready > b->first_ready;
    if (a->first_stock != b->first_stock) return a->first_stock > b->first_stock;
    if (a->first_items != b->first_items) return a->first_items > b->first_items;
    if (a->stock_used != b->stock_used) return a->stock_used > b->stock_used;
    return a->total < b->total;
}
// Every order of the stats is tried (the first pass finds the shortest route, the second keeps the best one within 3 breeds of it).
static void visit(Search *search, const unsigned char *order, unsigned count) {
    evaluate(search->state, order, count, &search->work);
    if (search->prices) {
        for (unsigned i=0;i<BREED_ITEMS;i++) {
            int available=search->state->have[i]>0?search->state->have[i]:0;
            int tobuy=search->work.plan.need[i]>available ? search->work.plan.need[i]-available:0;
            if (!tobuy)continue;
            if (search->prices[i])search->work.priced_cost += (uint64_t)tobuy*search->prices[i];
            else search->work.price_unknown += (unsigned)tobuy;
        }
        for (unsigned j=0;j<search->work.plan.count;j++)
            for(unsigned k=0;k<2;k++)if(search->work.plan.steps[j].parent[k].kind==0) {
                if(search->prices[BREED_ITEMS])search->work.priced_cost+=search->prices[BREED_ITEMS];
                else search->work.price_unknown++;
            }
    }
    int length = (int)search->work.plan.count;
    if (search->pass == 0) {
        if (length < search->shortest) search->shortest = length;
    } else if (length <= search->shortest + 3 && (!search->has_best || better(&search->work, &search->best,search->prices!=NULL))) {
        search->best = search->work;
        search->has_best = true;
    }
}
static void permute(Search *search, unsigned char *picked, unsigned picked_count, const unsigned char *rest, unsigned rest_count) {
    if (!rest_count) {
        visit(search, picked, picked_count);
        return;
    }
    for (unsigned i = 0; i < rest_count; ++i) {
        unsigned char next[BREED_STATS];
        unsigned count = 0;
        for (unsigned j = 0; j < rest_count; ++j)
            if (j != i) next[count++] = rest[j];
        picked[picked_count] = rest[i];
        permute(search, picked, picked_count + 1, next, rest_count - 1);
    }
}
const char *breedProblem(const BreedState *state) {
    if (!state->species[0]) return "No target Pokemon.";
    for (unsigned i = 0; i < sizeof(NO_BREED) / sizeof(NO_BREED[0]); ++i)
        if (strcasecmp(state->species, NO_BREED[i]) == 0) return "That Pokemon cannot breed (baby, legendary, Ditto, Unown or Shedinja).";
    if (!(state->ivs & ((1u << BREED_STATS) - 1u))) return "No 31 IV stats chosen.";
    return NULL;
}
static void breedPlanInternal(const BreedState *state, const uint32_t *prices, BreedPlan *plan) {
    unsigned char order[BREED_STATS], picked[BREED_STATS];
    unsigned count = 0;
    for (unsigned i = 0; i < BREED_STATS; ++i)
        if ((state->ivs >> i) & 1u) order[count++] = (unsigned char)i;
    for (unsigned i = 1; i < count; ++i) {  // the stats whose Power item is owned go last: they are used at the most steps
        unsigned char value = order[i];
        unsigned j = i;
        for (; j > 0 && state->have[order[j - 1]] > 0 && !(state->have[value] > 0); --j) order[j] = order[j - 1];
        order[j] = value;
    }
    static Search search;  // large: not on the stack
    memset(&search, 0, sizeof(search));
    search.state = state;
    search.prices = prices;
    search.shortest = 1 << 20;
    for (search.pass = 0; search.pass < 2; ++search.pass) {
        if (count > 2)
            permute(&search, picked, 0, order, count);
        else
            visit(&search, order, count);
    }
    *plan = search.best.plan;
    plan->extra = (int)plan->count - search.shortest;
}
void breedPlan(const BreedState *state, BreedPlan *plan) {breedPlanInternal(state,NULL,plan);}
void breedPlanPriced(const BreedState *state, const uint32_t costs[BREED_ITEMS+1], BreedPlan *plan) {
    bool has_price=false;
    if (costs)for (unsigned i=0;i<BREED_ITEMS+1;i++)if(costs[i])has_price=true;
    breedPlanInternal(state,has_price?costs:NULL,plan);
}
void breedShoppingList(const BreedState *state, const BreedPlan *plan, int to_buy[BREED_ITEMS]) {
    for (unsigned i = 0; i < BREED_ITEMS; ++i) {
        int available = state->have[i] > 0 ? state->have[i] : 0;
        to_buy[i] = plan->need[i] > available ? plan->need[i] - available : 0;
    }
}
static void append(char *out, size_t size, const char *format, ...) __attribute__((format(printf, 3, 4)));
static void append(char *out, size_t size, const char *format, ...) {
    size_t at = strlen(out);
    va_list args;
    va_start(args, format);
    if (at < size) vsnprintf(out + at, size - at, format, args);
    va_end(args);
}
void breedResult(const BreedState *state, const BreedStep *step, char *out, size_t size) {
    snprintf(out, size, "%ux31 (", step->count);
    for (unsigned i = 0; i < step->count; ++i) append(out, size, "%s%s", i ? "/" : "", BREED_STAT_NAMES[step->order[i]]);
    append(out, size, ")");
    if (step->natured) append(out, size, " + %s", BREED_NATURES[state->nature]);
}
void breedLabel(const BreedState *state, const BreedPlan *plan, BreedParent parent, char *out, size_t size) {
    if (parent.kind == 0) {
        if (parent.ref >= 0)
            snprintf(out, size, "31 %s", BREED_STAT_NAMES[parent.ref]);
        else
            snprintf(out, size, "any %s", BREED_NATURES[state->nature]);
    } else if (parent.kind == 1) {
        const BreedStock *mon = &state->stock[parent.ref];
        snprintf(out, size, "Your %s", mon->name);
        if (mon->nature >= 0) append(out, size, " (%s)", BREED_NATURES[mon->nature]);
    } else {
        char result[96];
        breedResult(state, &plan->steps[parent.ref], result, sizeof(result));
        snprintf(out, size, "R%d (%s)", parent.ref + 1, result);
    }
}
void breedDone(BreedState *state, const BreedPlan *plan) {
    if (!plan->count) return;
    const BreedStep *step = &plan->steps[0];
    BreedStock result = {.ivs = 0, .nature = step->natured ? state->nature : -1};
    for (unsigned i = 0; i < step->count; ++i) result.ivs |= 1u << step->order[i];
    if (plan->count == 1)
        snprintf(result.name, sizeof(result.name), "%s", state->species);
    else {
        snprintf(result.name, sizeof(result.name), "Bred ");
        for (unsigned i = 0; i < step->count; ++i) append(result.name, sizeof(result.name), "%s%s", i ? "/" : "", BREED_STAT_NAMES[step->order[i]]);
        if (step->natured) append(result.name, sizeof(result.name), " %s", BREED_NATURES[state->nature]);
    }
    int gone[2] = {step->parent[0].kind == 1 ? step->parent[0].ref : -1, step->parent[1].kind == 1 ? step->parent[1].ref : -1};
    if (gone[0] < gone[1]) {
        int swap = gone[0];
        gone[0] = gone[1], gone[1] = swap;
    }
    for (unsigned i = 0; i < 2; ++i) {  // the larger index first, so that the other does not move
        if (gone[i] < 0) continue;
        memmove(&state->stock[gone[i]], &state->stock[gone[i] + 1], (state->stock_count - (unsigned)gone[i] - 1) * sizeof(BreedStock));
        state->stock_count--;
    }
    for (unsigned i = 0; i < 2; ++i)
        if (step->have[i] && state->have[step->item[i]] > 0) state->have[step->item[i]]--;
    if (state->stock_count < BREED_STOCK) state->stock[state->stock_count++] = result;
}

// ---- the file: a line per fact, commas between the fields ----------------------------------------------------------------------
//   target,<species>,<nature>,<stats>      stats: a number, a bit per stat (HP 1, Atk 2, Def 4, SpA 8, SpD 16, Spe 32)
//   item,<number>,<how many>               number: 0 to 5 the Power items in the order of the stats, 6 the Everstone
//   stock,<name>,<stats>,<nature>
//   left,<breeds>                          only in a chain that was put aside
static int natureIndex(const char *name) {
    for (unsigned i = 0; i < BREED_NATURE_COUNT; ++i)
        if (strcasecmp(name, BREED_NATURES[i]) == 0) return (int)i;
    return -1;
}
bool breedLoad(const char *path, BreedState *state) {
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    memset(state, 0, sizeof(*state));
    state->nature = state->left = -1;
    char line[160];
    while (fgets(line, sizeof(line), file)) {
        char *field[4] = {line, (char *)"", (char *)"", (char *)""};
        unsigned count = 1;
        char *end = strpbrk(line, "\r\n");
        if (end) *end = 0;
        for (char *comma = strchr(line, ','); comma && count < 4; comma = strchr(comma, ',')) {
            *comma++ = 0;
            field[count++] = comma;
        }
        if (!strcmp(field[0], "target")) {
            snprintf(state->species, sizeof(state->species), "%s", field[1]);
            state->nature = natureIndex(field[2]);
            state->ivs = (unsigned)atoi(field[3]) & ((1u << BREED_STATS) - 1u);
        } else if (!strcmp(field[0], "left")) {
            state->left = atoi(field[1]);
        } else if (!strcmp(field[0], "item")) {
            int item = atoi(field[1]);
            if (item >= 0 && item < (int)BREED_ITEMS) state->have[item] = atoi(field[2]);
        } else if (!strcmp(field[0], "stock") && field[1][0] && state->stock_count < BREED_STOCK) {
            BreedStock *mon = &state->stock[state->stock_count++];
            snprintf(mon->name, sizeof(mon->name), "%s", field[1]);
            mon->ivs = (unsigned)atoi(field[2]) & ((1u << BREED_STATS) - 1u);
            mon->nature = natureIndex(field[3]);
        }
    }
    fclose(file);
    return true;
}
static void clean(char *out, const char *in, size_t size) {  // no commas or line breaks in a field
    snprintf(out, size, "%s", in);
    for (; *out; ++out)
        if (*out == ',' || *out == '\r' || *out == '\n') *out = ' ';
}
bool breedSave(const char *path, const BreedState *state) {
    char temporary[256], name[64];
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    FILE *file = fopen(temporary, "wb");
    if (!file) return false;
    clean(name, state->species, sizeof(name));
    fprintf(file, "target,%s,%s,%u\n", name, state->nature >= 0 ? BREED_NATURES[state->nature] : "", state->ivs);
    if (state->left >= 0) fprintf(file, "left,%d\n", state->left);
    for (unsigned i = 0; i < BREED_ITEMS; ++i)
        if (state->have[i] > 0) fprintf(file, "item,%u,%d\n", i, state->have[i]);
    for (unsigned i = 0; i < state->stock_count; ++i) {
        clean(name, state->stock[i].name, sizeof(name));
        fprintf(file, "stock,%s,%u,%s\n", name, state->stock[i].ivs, state->stock[i].nature >= 0 ? BREED_NATURES[state->stock[i].nature] : "");
    }
    bool ok = fclose(file) == 0;
    if (ok) remove(path), ok = rename(temporary, path) == 0;  // the new file is complete before the old one goes
    return ok;
}
