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
    int WindowID = -1;
    _AXUIElementGetWindow(Element, &WindowID);
    window_info *Window = GetWindowByID(WindowID);
    if(!Window)
        return;

    if(CFEqual(Notification, kAXTitleChangedNotification))
        Window->Name = GetWindowTitle(Element);

    if(IsWindowFloating(WindowID, NULL))
        UpdateBorder("focused");

    if(!IsWindowFloating(WindowID, NULL) &&
        KWMToggles.EnableDragAndDrop &&
        KWMToggles.DragInProgress &&
        CFEqual(Notification, kAXWindowMovedNotification))
    {
        KWMToggles.DragInProgress = false;
        KWMTiling.FloatingWindowLst.push_back(WindowID);
        RemoveWindowFromBSPTree(KWMScreen.Current, WindowID, false, false);

        if(KWMMode.Focus != FocusModeDisabled &&
           KWMMode.Focus != FocusModeAutofocus &&
           KWMToggles.StandbyOnFloat)
        {
            KWMMode.Focus = FocusModeStandby;
        }
    }
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
