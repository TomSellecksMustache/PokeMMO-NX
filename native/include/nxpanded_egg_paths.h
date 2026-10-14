#pragma once
#include "breeding.h"
#include <stdbool.h>
#include <stddef.h>

#define NX_EGG_ROUTES_MAX 128u
#define NX_EGG_ROUTE_HOPS 5u
#define NX_EGG_MOVE_LEN 52u

typedef struct { char receiver[40], move[NX_EGG_MOVE_LEN], donor[40]; bool donor_knows_move; } NxEggEdge;
typedef struct { NxEggEdge edge[NX_EGG_ROUTES_MAX]; unsigned count; unsigned rejected; } NxEggRoutes;
typedef struct { char species[NX_EGG_ROUTE_HOPS+1][40]; unsigned hops; bool owned_source; } NxEggPath;
/* TSV: receiver<TAB>move<TAB>donor<TAB>known|inherited<TAB>verified.
 * All relations require explicit user-curated verification, not move-name matching.
 * Invalid routes and incompatible egg groups are rejected. */
bool nxEggRoutesLoad(const char *filename, NxEggRoutes *routes);
/* Find shortest complete donor path (terminal donor knows the move), with
 * available owned terminal donors preferred when depth is equal. */
bool nxEggFindPath(const NxEggRoutes *routes, const BreedState *stock,
                   const char *target, const char *move, NxEggPath *path);
