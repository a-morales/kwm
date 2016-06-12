#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "types.h"
#include "axlib/display.h"

void LoadBSPTreeFromFile(ax_display *Display, space_info *SpaceInfo, std::string Name);

void SaveBSPTreeToFile(screen_info *Screen, std::string Name);
void LoadBSPTreeFromFile(screen_info *Screen, std::string Name);


#endif
