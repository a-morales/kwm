#ifndef BORDER_H
#define BORDER_H

#include "types.h"

std::string ConvertHexToRGBAString(int WindowID, int Color, int Width);
kwm_time_point PerformUpdateBorderTimer(kwm_time_point Time);

void OpenBorder(kwm_border *Border)
void RefreshBorder(kwm_border *Border, int WindowID);
void ClearBorder(kwm_border *Border);
void CloseBorder(kwm_border *Border);

void UpdateBorder(std::string BorderType);

#endif
