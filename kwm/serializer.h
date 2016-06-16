#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "types.h"
#include "axlib/display.h"

void LoadBSPTreeFromFile(ax_display *Display, space_info *SpaceInfo, std::string Name);
void SaveBSPTreeToFile(ax_display *Display, space_info *SpaceInfo, std::string Name);

#endif
