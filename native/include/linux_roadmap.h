#pragma once
#include "linux_sdl_events.h"
#include <stdbool.h>
#include <stdint.h>

// The breeding roadmap, drawn over the game like the file chooser: R + D-pad right + right stick opens it. The target, the items and the Pokemon
// the player owns are set in the roadmap itself (+) and kept in switch/PokeMMO-NXpanded/breeding.csv. It shows the next breed and updates the file as breeds are done.
// A: done (asks first), Y: items, X: all steps, +: setup, B: close. The game sees the controller at rest while it is open.
typedef void (*LinuxRoadmapRect)(float x, float y, float w, float h, float r, float g, float b, float a);
typedef void (*LinuxRoadmapText)(float x, float y, const char *string, unsigned max_chars, float r, float g, float b);
typedef void (*LinuxRoadmapClip)(float x, float y, float w, float h);

// Used by the SDL layer.
void linuxRoadmapShow(void);
void linuxRoadmapShowVisual(void); // R + D-pad down + R3: graphics options, non-client native menu
void linuxRoadmapShowShopping(void);  // R + D-pad left + R3: second tab of the main roadmap; opens Shopping directly
void linuxRoadmapToastShow(uint64_t now_ns); // R + D-pad up + R3: non-modal progress, no keyboard, no input capture
float linuxRoadmapToastOpacity(uint64_t now_ns); // fade linearly from 1 to 0 over 10 seconds
void linuxRoadmapToastDraw(LinuxRoadmapRect rect, LinuxRoadmapText text, LinuxRoadmapClip clip, float opacity);                               // loads the file and opens the roadmap
bool linuxRoadmapOpen(void);                               // it is on the screen
bool linuxRoadmapCapturesInput(void);                      // it is open, or its closing buttons have not been released yet
void linuxRoadmapFeed(const LinuxInputSnapshot *snapshot);  // the sampled controller, while it captures it
void linuxRoadmapDraw(LinuxRoadmapRect rect, LinuxRoadmapText text, LinuxRoadmapClip clip);  // on the rendering thread, with the file chooser's drawing state
