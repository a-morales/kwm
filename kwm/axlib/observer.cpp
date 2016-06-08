#include "observer.h"
#include "application.h"

#define internal static
#define local_persist static

void AXLibConstructObserver(ax_application *Application, ObserverCallback Callback)
{
    AXError Result = AXObserverCreate(Application->PID, Callback, &Application->Observer.Ref);
    Application->Observer.Valid = (Result == kAXErrorSuccess);
    Application->Observer.Application = Application;
}

void AXLibStartObserver(ax_observer *Observer)
{
    CFRunLoopAddSource(CFRunLoopGetCurrent(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode);
}

AXError AXLibAddObserverNotification(ax_observer *Observer, AXUIElementRef Ref, CFStringRef Notification, void *Reference)
{
    return AXObserverAddNotification(Observer->Ref, Ref, Notification, Reference);
}

void AXLibRemoveObserverNotification(ax_observer *Observer, AXUIElementRef Ref, CFStringRef Notification)
{
    AXObserverRemoveNotification(Observer->Ref, Ref, Notification);
}

void AXLibStopObserver(ax_observer *Observer)
{
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode);
}

void AXLibDestroyObserver(ax_observer *Observer)
{
    CFRelease(Observer->Ref);
    Observer->Valid = false;
    Observer->Ref = NULL;
    Observer->Application = NULL;
}
