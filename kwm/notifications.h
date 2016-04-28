#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include "types.h"

void FocusedAXObserverCallback(AXObserverRef Observer, AXUIElementRef Element, CFStringRef Notification, void *ContextData);
void MarkedAXObserverCallback(AXObserverRef Observer, AXUIElementRef Element, CFStringRef Notification, void *ContextData);

void CreateFocusedWindowNotifications();
void DestroyFocusedWindowNotifications();

void CreateMarkedWindowNotifications();
void DestroyMarkedWindowNotifications();

#endif
