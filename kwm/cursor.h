#ifndef CURSOR_H
#define CURSOR_H

#include "axlib/window.h"
#include <Carbon/Carbon.h>

void FocusWindowBelowCursor();
void MoveCursorToCenterOfWindow(ax_window *Window);
void MoveCursorToCenterOfFocusedWindow();

#endif
