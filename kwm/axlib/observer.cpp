#include "observer.h"
#include "application.h"

#define internal static
#define local_persist static

/* NOTE(koekeishiya): For compatibility with current Kwm code
 * TODO: Remove.
 */
ax_observer AXLibConstructObserver(int PID, ObserverCallback Callback)
{
    ax_observer Observer = {};

    AXObserverCreate(PID, Callback, &Observer.Ref);
    Observer.AppRef = AXUIElementCreateApplication(PID);

    return Observer;
}

void AXLibAddObserverNotificationOLD(ax_observer *Observer, CFStringRef Notification, void *Reference)
{
    AXObserverAddNotification(Observer->Ref, Observer->AppRef, Notification, Reference);
}

void AXLibRemoveObserverNotificationOLD(ax_observer *Observer, CFStringRef Notification)
{
    AXObserverRemoveNotification(Observer->Ref, Observer->AppRef, Notification);
}

/* END */

void AXLibConstructObserver(ax_application *Application, ObserverCallback Callback)
{
    AXError Result = AXObserverCreate(Application->PID, Callback, &Application->Observer.Ref);
    Application->Observer.Valid = (Result == kAXErrorSuccess);
    Application->Observer.Application = Application;
}

void AXLibStartObserver(ax_observer *Observer)
{
    CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode);
}

void AXLibAddObserverNotification(ax_observer *Observer, CFStringRef Notification, void *Reference)
{
    AXObserverAddNotification(Observer->Ref, Observer->Application->Ref, Notification, Reference);
}

void AXLibRemoveObserverNotification(ax_observer *Observer, CFStringRef Notification)
{
    AXObserverRemoveNotification(Observer->Ref, Observer->Application->Ref, Notification);
}

void AXLibStopObserver(ax_observer *Observer)
{
    CFRunLoopRemoveSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode);
}

void AXLibDestroyObserver(ax_observer *Observer)
{
    CFRelease(Observer->Ref);
    Observer->Valid = false;
    Observer->Ref = NULL;
    Observer->Application = NULL;
}
