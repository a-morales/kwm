#ifndef KWM_H
#define KWM_H

#include "types.h"

extern "C" bool CGSIsSecureEventInputSet(void);
extern "C" void NSApplicationLoad(void);
extern void CreateWorkspaceWatcher(void *Watcher);

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon);

void KwmQuit();
void KwmReloadConfig();

#endif
