#pragma once
#include "linux_sdl_events.h"
#include <stdio.h>

// Reads the console's input (controller, touch screen) into a snapshot for linux_sdl_events.c. Switch only.
bool linuxSdlInputSample(LinuxInputSnapshot *snapshot);
// The console's software keyboard (swkbd): blocks until the player confirms or cancels. UTF-8 text in `out`; false when cancelled.
// The inline software keyboard: the applet draws it over the game, which keeps running. The game is told what is typed with
// key and text events (linuxSdlEventsTextChanged). Request it with linuxSdlInputKeyboardRequest; linuxSdlInputKeyboardPump
// (called every frame from the SDL event pump) moves it along without ever blocking.
void linuxSdlInputKeyboardRequest(bool show);
void linuxSdlInputKeyboardPump(void);
bool linuxSdlInputKeyboardVisible(void);
// While a target is set, what is typed goes to it and not to the game: done is 0 for a change of the text, 1 for enter and 2 for cancel.
void linuxSdlInputKeyboardTarget(void (*target)(const char *text, int done));
bool linuxSdlInputKeyboardAvailable(void);  // false when the inline keyboard cannot be started (the modal one is used instead)
