#pragma once
#include <stdbool.h>
// A requested EGL surface sample count, applied on the next graphics-context creation.
unsigned linuxMsaaRequested(void);
bool linuxMsaaSetRequested(unsigned samples);
void linuxMsaaSetActive(unsigned samples);
unsigned linuxMsaaActive(void);
