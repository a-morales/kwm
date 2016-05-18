#include "observer.h"

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

void AXLibAddObserverNotification(ax_observer *Observer, CFStringRef Notification)
{
    AXObserverAddNotification(Observer->Ref, Observer->AppRef, Notification, NULL);
}

void AXLibRemoveObserverNotification(ax_observer *Observer, CFStringRef Notification)
{
    AXObserverRemoveNotification(Observer->Ref, Observer->AppRef, Notification);
}

/* END */

ax_observer AXLibConstructObserver(ax_application *Application, ObserverCallback Callback)
{
    ax_observer Observer = {};

    AXObserverCreate(Application->PID, Callback, &Observer.Ref);
    Observer.Application = Application;

    return Observer;
}

void AXLibStartObserver(ax_observer *Observer)
{
    CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode);
}

/* NOTE(koekeishiya): These functions will replace the temporary code above
void AXLibAddObserverNotification(ax_observer *Observer, CFStringRef Notification)
{
    AXObserverAddNotification(Observer->Ref, Observer->Application->Ref, Notification, NULL);
}

void AXLibRemoveObserverNotification(ax_observer *Observer, CFStringRef Notification)
{
    AXObserverRemoveNotification(Observer->Ref, Observer->Application->Ref, Notification);
}
*/

void AXLibStopObserver(ax_observer *Observer)
{
    CFRunLoopRemoveSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(Observer->Ref), kCFRunLoopDefaultMode);
}

void AXLibDestroyObserver(ax_observer *Observer)
{
    CFRelease(Observer->Ref);
    Observer->Ref = NULL;
    Observer->Application = NULL;
}
