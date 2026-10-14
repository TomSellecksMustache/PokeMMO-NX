#pragma once
#include <stdbool.h>
#include <stddef.h>
// Downloads only newer stable NRO assets to a dedicated staging folder,
// checks SHA256 and leaves installation to the user after NXpanded exits.
bool linuxUpdaterCheckAndStage(char *message, size_t cap);
