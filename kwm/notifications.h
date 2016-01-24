#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include "types.h"

void FocusedAXObserverCallback(AXObserverRef Observer, AXUIElementRef Element, CFStringRef Notification, void *ContextData);

void CreateApplicationNotifications();
void DestroyApplicationNotifications();

#endif
