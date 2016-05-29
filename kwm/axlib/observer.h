#ifndef AXLIB_OBSERVER_H
#define AXLIB_OBSERVER_H

#include <Carbon/Carbon.h>
#include "types.h"

#define OBSERVER_CALLBACK(name) void name(AXObserverRef Observer, AXUIElementRef Element,\
                                          CFStringRef Notification, void *Reference)
typedef OBSERVER_CALLBACK(ObserverCallback);

struct ax_observer
{
    ax_application *Application;
    AXObserverRef Ref;
    bool Valid;

    /* TODO(koekeishiya): Remove after implementing ax_application */
    AXUIElementRef AppRef;
};

void AXLibConstructObserver(ax_application *Application, ObserverCallback Callback);
void AXLibDestroyObserver(ax_observer *Observer);

void AXLibStartObserver(ax_observer *Observer);
void AXLibStopObserver(ax_observer *Observer);

void AXLibAddObserverNotification(ax_observer *Observer, CFStringRef Notification, void *Reference);
void AXLibRemoveObserverNotification(ax_observer *Observer, CFStringRef Notification);

/* TODO(koekeishiya): Remove after implementing ax_application */
ax_observer AXLibConstructObserver(pid_t PID, ObserverCallback Callback);
void AXLibAddObserverNotificationOLD(ax_observer *Observer, CFStringRef Notification, void *Reference);
void AXLibRemoveObserverNotificationOLD(ax_observer *Observer, CFStringRef Notification);

#endif
