#pragma once
#include <stdbool.h>
#include <stdint.h>

// Normalize transient controller ownership before forwarding SDL state.
uint32_t linuxSdlInputPreprocess(uint32_t buttons, uint64_t timestamp_ns, bool *changed);
void linuxSdlInputPreprocessReset(void);
