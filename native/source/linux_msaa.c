#include "linux_msaa.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/stat.h>
#define MSAA_FILE "sdmc:/switch/PokeMMO-NXpanded/msaa.cfg"
static atomic_uint requested, active;
static atomic_bool loaded;
static unsigned normalize(unsigned n) { return n == 2 || n == 4 || n == 8 ? n : 0; }
unsigned linuxMsaaRequested(void) {
    if (!atomic_load(&loaded)) {
        unsigned n = 0;
        FILE *f = fopen(MSAA_FILE, "rb");
        if (f) { if (fscanf(f, "msaa=%u", &n) != 1) n = 0; fclose(f); }
        atomic_store(&requested, normalize(n));
        atomic_store(&loaded, true);
    }
    return atomic_load(&requested);
}
bool linuxMsaaSetRequested(unsigned samples) {
    if (samples != normalize(samples)) return false;
    mkdir("sdmc:/switch/PokeMMO-NXpanded", 0777);
    FILE *f = fopen(MSAA_FILE ".tmp", "wb");
    if (!f) return false;
    bool ok = fprintf(f, "msaa=%u\n", samples) > 0;
    if (fflush(f)) ok = false;
    if (fclose(f)) ok = false;
    if (!ok) { remove(MSAA_FILE ".tmp"); return false; }
    // The Switch filesystem cannot be assumed to overwrite with rename().
    remove(MSAA_FILE ".bak");
    bool had = rename(MSAA_FILE, MSAA_FILE ".bak") == 0;
    if (rename(MSAA_FILE ".tmp", MSAA_FILE) != 0) {
        if (had) rename(MSAA_FILE ".bak", MSAA_FILE);
        remove(MSAA_FILE ".tmp");
        return false;
    }
    remove(MSAA_FILE ".bak");
    atomic_store(&requested, samples);
    atomic_store(&loaded, true);
    return true;
}
void linuxMsaaSetActive(unsigned samples) { atomic_store(&active, samples); }
unsigned linuxMsaaActive(void) { return atomic_load(&active); }
