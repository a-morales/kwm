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
    Assert(Element, "AXOBserverCallback() Element was null")

    window_info *Window = KWMFocus.Window;
    if(!Window)
        return;

    if(CFEqual(Notification, kAXTitleChangedNotification))
        Window->Name = GetWindowTitle(Element);
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
        int ElementWID = -1;
        _AXUIElementGetWindow(Element, &ElementWID);
        if(Window->WID != ElementWID)
        {
            window_info *ElementWindow = GetWindowByID(ElementWID);
            if(ElementWindow)
            {
                SetWindowRefFocus(Element, ElementWindow);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }

    if(!IsWindowFloating(Window->WID, NULL) &&
        KWMToggles.EnableDragAndDrop &&
        KWMToggles.DragInProgress &&
        CFEqual(Notification, kAXWindowMovedNotification))
    {
        KWMToggles.DragInProgress = false;
        KWMTiling.FloatingWindowLst.push_back(Window->WID);
        RemoveWindowFromBSPTree(KWMScreen.Current, Window->WID, false, false);

        if(KWMMode.Focus != FocusModeDisabled &&
           KWMMode.Focus != FocusModeAutofocus &&
           KWMToggles.StandbyOnFloat)
        {
            KWMMode.Focus = FocusModeStandby;
        }
    }

    if(IsWindowFloating(Window->WID, NULL) ||
       IsApplicationFloating(Window))
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
    AXObserverRemoveNotification(KWMFocus.Observer, KWMFocus.Application, kAXFocusedWindowChangedNotification);
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
        AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXFocusedWindowChangedNotification, NULL);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), AXObserverGetRunLoopSource(KWMFocus.Observer), kCFRunLoopDefaultMode);
    }
}
