#ifndef SPACE_H
#define SPACE_H

#include "types.h"

bool GetTagForCurrentSpace(std::string &Tag);
bool IsSpaceInitializedForScreen(screen_info *Screen);
bool DoesSpaceExistInMapOfScreen(screen_info *Screen);

bool IsSpaceSystemOrFullscreen();
bool IsSpaceTransitionInProgress();

bool IsSpaceFloating(int SpaceID);
bool IsActiveSpaceFloating();

void TileFocusedSpace(space_tiling_option Mode);
void FloatFocusedSpace();
void ToggleFocusedSpaceFloating();

#endif
