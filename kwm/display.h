#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

extern int GetActiveSpaceOfDisplay(screen_info *Screen);

void DisplayReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void *UserInfo);
void GetActiveDisplays();
void RefreshActiveDisplays();

int GetIndexOfNextScreen();
int GetIndexOfPrevScreen();

screen_info *GetDisplayFromScreenID(unsigned int ID);
screen_info *GetDisplayOfMousePointer();
screen_info *GetDisplayOfWindow(window_info *Window);

std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex);
void MoveWindowToDisplay(window_info *Window, int Shift, bool Relative, bool UpdateFocus);

void GiveFocusToScreen(unsigned int ScreenIndex, tree_node *Focus, bool Mouse, bool UpdateFocus);
void UpdateActiveScreen();

container_offset CreateDefaultScreenOffset();
screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex);
void UpdateExistingScreenInfo(screen_info *Screen, int DisplayIndex, int ScreenIndex);

void SetDefaultPaddingOfDisplay(container_offset Offset);
void SetDefaultGapOfDisplay(container_offset Offset);
void ChangePaddingOfDisplay(const std::string &Side, int Offset);
void ChangeGapOfDisplay(const std::string &Side, int Offset);
space_settings *GetSpaceSettingsForDisplay(unsigned int ScreenID);

#endif
