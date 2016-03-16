#include "notifications.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "border.h"

extern kwm_screen KWMScreen;
extern kwm_toggles KWMToggles;
extern kwm_tiling KWMTiling;
extern kwm_focus KWMFocus;
extern kwm_thread KWMThread;
extern kwm_mode KWMMode;

void FocusedAXObserverCallback(AXObserverRef Observer, AXUIElementRef Element, CFStringRef Notification, void *ContextData)
{
    pthread_mutex_lock(&KWMThread.Lock);

    window_info *Window = KWMFocus.Window;
    if(Window && CFEqual(Notification, kAXTitleChangedNotification))
        Window->Name = GetWindowTitle(Element);
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
        if(!Window || Window->WID != GetWindowIDFromRef(Element))
        {
            DEBUG("Element: " << GetWindowTitle(Element))
            SetKwmFocus(Element);
            screen_info *Screen = GetDisplayOfWindow(KWMFocus.Window);
            if(Screen && KWMScreen.Current != Screen)
            {
                KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
                KWMScreen.Current = Screen;
                KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
                ShouldActiveSpaceBeManaged();
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
    else if(CFEqual(Notification, kAXWindowResizedNotification) ||
            CFEqual(Notification, kAXWindowMovedNotification) ||
            CFEqual(Notification, kAXWindowMiniaturizedNotification))
    {
        UpdateBorder("focused");
        if (Window && Window->WID == KWMScreen.MarkedWindow)
            UpdateBorder("marked");
    }

    pthread_mutex_unlock(&KWMThread.Lock);
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
    CFRunLoopRemoveSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(KWMFocus.Observer), kCFRunLoopDefaultMode);

    CFRelease(KWMFocus.Observer);
    KWMFocus.Observer = NULL;
    CFRelease(KWMFocus.Application);
    KWMFocus.Application = NULL;
}

void CreateApplicationNotifications()
{
    DestroyApplicationNotifications();

    if(KWMFocus.Window)
    {
        KWMFocus.Application = AXUIElementCreateApplication(KWMFocus.Window->PID);
        if(!KWMFocus.Application)
            return;

        AXError Error = AXObserverCreate(KWMFocus.Window->PID, FocusedAXObserverCallback, &KWMFocus.Observer);
        if(Error == kAXErrorSuccess)
        {
            AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowMiniaturizedNotification, NULL);
            AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowMovedNotification, NULL);
            AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXWindowResizedNotification, NULL);
            AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXTitleChangedNotification, NULL);
            AXObserverAddNotification(KWMFocus.Observer, KWMFocus.Application, kAXFocusedWindowChangedNotification, NULL);
            CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(KWMFocus.Observer), kCFRunLoopDefaultMode);
        }
    }
}
