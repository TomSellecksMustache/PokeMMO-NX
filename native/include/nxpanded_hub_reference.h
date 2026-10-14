#pragma once
#include <stddef.h>
#include <stdbool.h>
#define NX_HUB_MOVES_MAX 5
#define NX_HUB_TEXT_LENGTH 52

typedef struct {
    char move[NX_HUB_TEXT_LENGTH];
    char donor[NX_HUB_TEXT_LENGTH];
} NxHubEggMove;
// Optional SD reference file. Never reads the PokeMMO client.
// TSV: species<TAB>egg move<TAB>potential donor (validated by the player).
size_t nxHubEggMoves(const char *path, const char *species, NxHubEggMove *out, size_t cap);
