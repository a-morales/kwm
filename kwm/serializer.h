#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "types.h"

void SaveBSPTreeToFile(screen_info *Screen, std::string Name);
void LoadBSPTreeFromFile(screen_info *Screen, std::string Name);

#endif
