#ifndef BORDER_H
#define BORDER_H

#include "types.h"

std::string ConvertHexToRGBAString(int WindowID, int Color, int Width);
kwm_time_point PerformUpdateBorderTimer(kwm_time_point Time);
void CloseBorder(kwm_border &Border);
void UpdateBorder(std::string BorderType);
void ClearBorder(kwm_border &Border);

#endif
