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
    if(!CFRunLoopContainsSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode))
        CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode);
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
    /* NOTE(koekeishiya): CFRunLoopSourceInvalidate removes the source from all run-loops, ignoring run-loops that no longer exist.
                          Replaces CFRunLoopRemoveSource(CFRunLoopGetCurrent(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode); */
    CFRunLoopSourceInvalidate(AXObserverGetRunLoopSource(Observer->Ref));
}

void AXLibDestroyObserver(ax_observer *Observer)
{
    CFRelease(Observer->Ref);
    Observer->Valid = false;
    Observer->Ref = NULL;
    Observer->Application = NULL;
}
