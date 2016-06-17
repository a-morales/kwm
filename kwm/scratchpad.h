#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#include "types.h"
#include "axlib/axlib.h"

bool IsScratchpadSlotValid(int Index);
int GetScratchpadSlotOfWindow(ax_window *Window);
bool IsWindowOnScratchpad(ax_window *Window);
int GetFirstAvailableScratchpadSlot();
void AddWindowToScratchpad(ax_window *Window);
void RemoveWindowFromScratchpad(ax_window *Window);
void ToggleScratchpadWindow(int Index);
void HideScratchpadWindow(int Index);
void ShowScratchpadWindow(int Index);
void ResizeScratchpadWindow(ax_display *Display, ax_window *Window);
std::string GetWindowsOnScratchpad();
void ShowAllScratchpadWindows();

#endif
