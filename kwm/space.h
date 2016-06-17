#ifndef SPACE_H
#define SPACE_H

#include "types.h"
#include "axlib/axlib.h"

void GetTagForMonocleSpace(space_info *Space, std::string &Tag);
void GetTagForCurrentSpace(std::string &Tag);

void GoToPreviousSpace(bool MoveFocusedWindow);
space_settings *GetSpaceSettingsForDesktopID(int ScreenID, int DesktopID);
int GetSpaceFromName(ax_display *Display, std::string Name);
void SetNameOfActiveSpace(ax_display *Display, std::string Name);
std::string GetNameOfSpace(ax_display *Display, ax_space *Space);

void ActivateSpaceWithoutTransition(std::string SpaceID);
void MoveWindowBetweenSpaces(ax_display *Display, int SourceSpaceID, int DestinationSpaceID, uint32_t WindowID);
void MoveFocusedWindowToSpace(std::string SpaceID);

#endif
