#ifndef BORDER_H
#define BORDER_H

#include "types.h"

void OpenBorder(kwm_border *Border);
void RefreshBorder(kwm_border *Border, int WindowID);
void ClearBorder(kwm_border *Border);
void CloseBorder(kwm_border *Border);

void UpdateBorder(std::string BorderType);

#endif
