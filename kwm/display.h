#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

void DisplayReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void *UserInfo);
void GetActiveDisplays();
void RefreshActiveDisplays();

int GetIndexOfNextScreen();
int GetIndexOfPrevScreen();

screen_info *GetDisplayFromScreenID(unsigned int ID);
screen_info *GetDisplayOfMousePointer();
screen_info *GetDisplayOfWindow(window_info *Window);

std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex);
std::vector<int> GetAllWindowIDsOnDisplay(int ScreenIndex);
void CaptureApplicationToScreen(int ScreenID, std::string Application);
void MoveWindowToDisplay(window_info *Window, int Shift, bool Relative);

void SetSpaceModeOfDisplay(unsigned int ScreenIndex, std::string Mode);
space_tiling_option GetSpaceModeOfDisplay(unsigned int ScreenIndex);

void ActivateScreen(screen_info *Screen, bool Mouse);
void GiveFocusToScreen(int ScreenIndex, tree_node *Focus, bool Mouse);

container_offset CreateDefaultScreenOffset();
screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex);
void UpdateExistingScreenInfo(screen_info *Screen, int DisplayIndex, int ScreenIndex);

void SetDefaultPaddingOfDisplay(const std::string &Side, int Offset);
void SetDefaultGapOfDisplay(const std::string &Side, int Offset);
void ChangePaddingOfDisplay(const std::string &Side, int Offset);
void ChangeGapOfDisplay(const std::string &Side, int Offset);

#endif
