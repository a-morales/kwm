#ifndef APPLICATION_H
#define APPLICATION_H

#include "types.h"

void AllowRoleForApplication(std::string Application, std::string Role);
bool IsAppSpecificWindowRole(window_info *Window, CFTypeRef Role, CFTypeRef SubRole);
bool IsApplicationFloating(window_info *Window);
bool IsApplicationCapturedByScreen(window_info *Window);
bool IsApplicationInCache(int PID, std::vector<AXUIElementRef> *Elements);
bool GetWindowRefFromCache(window_info *Window, AXUIElementRef *WindowRef);
void FreeWindowRefCache(int PID);

#endif
