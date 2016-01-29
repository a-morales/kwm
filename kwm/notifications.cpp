#include "notifications.h"
#include "window.h"
#include "border.h"

extern kwm_screen KWMScreen;
extern kwm_toggles KWMToggles;
extern kwm_tiling KWMTiling;
extern kwm_focus KWMFocus;
extern kwm_mode KWMMode;

void FocusedAXObserverCallback(AXObserverRef Observer, AXUIElementRef Element, CFStringRef Notification, void *ContextData)
{
    if(!KWMFocus.Window)
    {
        DestroyApplicationNotifications();
        return;
    }

    if(KWMToggles.EnableDragAndDrop &&
       KWMToggles.DragInProgress &&
       CFEqual(Notification, kAXWindowMovedNotification) &&
       !IsWindowFloating(KWMFocus.Window->WID, NULL))
    {
        KWMToggles.DragInProgress = false;
        KWMTiling.FloatingWindowLst.push_back(KWMFocus.Window->WID);
        RemoveWindowFromBSPTree(KWMScreen.Current, KWMFocus.Window->WID, false, false);

        if(KWMMode.Focus != FocusModeDisabled && KWMMode.Focus != FocusModeAutofocus && KWMToggles.StandbyOnFloat)
            KWMMode.Focus = FocusModeStandby;
    }

    if(CFEqual(Notification, kAXTitleChangedNotification) && KWMFocus.Window)
        KWMFocus.Window->Name = GetWindowTitle(Element);

    if(IsFocusedWindowFloating())
        UpdateBorder("focused");
}

void DestroyApplicationNotifications()
{
    if(!KWMFocus.Observer)
        return;

    AXObserverRemoveNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowMiniaturizedNotification);
    AXObserverRemoveNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowMovedNotification);
    AXObserverRemoveNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowResizedNotification);
    AXObserverRemoveNotification(KWMFocus.Observer, KWMFocus.Application, kAXTitleChangedNotification);
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), AXObserverGetRunLoopSource(KWMFocus.Observer), kCFRunLoopDefaultMode);

    CFRelease(KWMFocus.Observer);
    KWMFocus.Observer = NULL;
    CFRelease(KWMFocus.Application);
    KWMFocus.Application = NULL;
}

void CreateApplicationNotifications()
{
    if(KWMFocus.Window)
    {
        AXObserverCreate(KWMFocus.Window->PID, FocusedAXObserverCallback, &KWMFocus.Observer);
        AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowMiniaturizedNotification, NULL);
        AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowMovedNotification, NULL);
        AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowResizedNotification, NULL);
        AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXTitleChangedNotification, NULL);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), AXObserverGetRunLoopSource(KWMFocus.Observer), kCFRunLoopDefaultMode);
    }
}
