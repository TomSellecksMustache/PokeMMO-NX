#pragma once
#include <stdint.h>
#include <stdbool.h>
static inline bool linuxHubHold(uint64_t now, bool down, uint64_t *since, bool *latched) {
    if (!down) { *since=0;*latched=false;return false; }
    if (!*since || now<*since) *since=now?now:1;
    if (!*latched && now>=*since && now-*since>=3000000000ull) {
        *latched=true;
        return true;
    }
    return false;
}
