#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#include "types.h"

bool IsScratchpadSlotValid(int Index);
int GetScratchpadSlotOfWindow(window_info *Window);
bool IsWindowOnScratchpad(window_info *Window);
int GetFirstAvailableScratchpadSlot();
void AddWindowToScratchpad(window_info *Window);
void RemoveWindowFromScratchpad(window_info *Window);
void ToggleScratchpadWindow(int Index);
void HideScratchpadWindow(int Index);
void ShowScratchpadWindow(int Index);
void ResizeScratchpadWindow(screen_info *Screen, window_info *Window);
std::string GetWindowsOnScratchpad();

#endif
