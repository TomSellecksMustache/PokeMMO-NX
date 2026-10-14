#include "nxpanded_hub_reference.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool equalSpecies(const char *a, const char *b) {
    while (*a && *b) {
        while (*a == ' ' || *a == '-') ++a;
        while (*b == ' ' || *b == '-') ++b;
        if (!*a || !*b) break;
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        ++a; ++b;
    }
    while (*a == ' ' || *a == '-') ++a;
    while (*b == ' ' || *b == '-') ++b;
    return !*a && !*b;
}
size_t nxHubEggMoves(const char *path, const char *species, NxHubEggMove *out, size_t cap) {
    if (!path || !species || !*species || !out || !cap) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char line[512];
    size_t count = 0;
    while (count < cap && fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char *name = line, *move = strchr(name, '\t');
        if (!move) continue;
        *move++ = 0;
        char *donor = strchr(move, '\t');
        if (!donor) continue;
        *donor++ = 0;
        donor[strcspn(donor, "\r\n")] = 0;
        if (!equalSpecies(name, species) || !*move) continue;
        // Reject newlines/control characters and visibly bound all fields.
        snprintf(out[count].move, sizeof(out[count].move), "%.*s", NX_HUB_TEXT_LENGTH-1, move);
        snprintf(out[count].donor, sizeof(out[count].donor), "%.*s", NX_HUB_TEXT_LENGTH-1, donor);
        ++count;
    }
    fclose(f);
    return count;
}
