#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"
#include "axlib/axlib.h"

void UpdateSpaceOfDisplay(ax_display *Display, space_info *Space);
void SetDefaultPaddingOfDisplay(container_offset Offset);
void SetDefaultGapOfDisplay(container_offset Offset);
void ChangePaddingOfDisplay(const std::string &Side, int Offset);
void ChangeGapOfDisplay(const std::string &Side, int Offset);
space_settings *GetSpaceSettingsForDisplay(unsigned int ScreenID);
container_offset CreateDefaultScreenOffset();

/* TODO(koekeishiya): Make this work for ax_window */
void MoveWindowToDisplay(window_info *Window, int Shift, bool Relative);

#endif
