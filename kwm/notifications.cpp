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
            window_info *OSXWindow = GetWindowByID(GetWindowIDFromRef(Element));
            screen_info *OSXScreen = GetDisplayOfWindow(OSXWindow);
            if(OSXWindow && OSXScreen)
            {
                screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
                bool SameScreen = ScreenOfWindow == OSXScreen;
                if(ScreenOfWindow && !SameScreen)
                    UpdateActiveWindowList(ScreenOfWindow);

                if(ScreenOfWindow && Window &&
                   GetWindowByID(Window->WID) == NULL &&
                   !SameScreen)
                {
                    space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
                    if(SpaceOfWindow->Settings.Mode == SpaceModeBSP)
                        RemoveWindowFromBSPTree(ScreenOfWindow, Window->WID, false, false);
                    else if(SpaceOfWindow->Settings.Mode == SpaceModeMonocle)
                        RemoveWindowFromMonocleTree(ScreenOfWindow, Window->WID, false, false);

                    SpaceOfWindow->FocusedWindowID = -1;
                }

                if(!SameScreen)
                    GiveFocusToScreen(OSXScreen->ID, NULL, false, false);

                SetKwmFocus(Element);
                if(SameScreen && !IsFocusedWindowFloating())
                    KWMFocus.InsertionPoint = KWMFocus.Cache;
            }
        }
    }
    else if(CFEqual(Notification, kAXWindowResizedNotification) ||
            CFEqual(Notification, kAXWindowMovedNotification))
    {
        if(KWMTiling.LockToContainer)
            LockWindowToContainerSize(Window);

        UpdateBorder("focused");
    }
    else if(CFEqual(Notification, kAXUIElementDestroyedNotification) ||
            CFEqual(Notification, kAXWindowMiniaturizedNotification))
    {
        UpdateBorder("focused");
    }

    pthread_mutex_unlock(&KWMThread.Lock);
}

void MarkedAXObserverCallback(AXObserverRef Observer, AXUIElementRef Element, CFStringRef Notification, void *ContextData)
{
    pthread_mutex_lock(&KWMThread.Lock);

    window_info *Window = &KWMScreen.MarkedWindow;
    if(Window->WID != 0 && Window->WID != -1)
    {
        if(CFEqual(Notification, kAXWindowResizedNotification) ||
           CFEqual(Notification, kAXWindowMovedNotification))
            UpdateBorder("marked");
        else if(CFEqual(Notification, kAXUIElementDestroyedNotification) ||
                CFEqual(Notification, kAXWindowMiniaturizedNotification))
            UpdateBorder("marked");
    }

    pthread_mutex_unlock(&KWMThread.Lock);
}

void CreateMarkedWindowNotifications()
{
    DestroyMarkedWindowNotifications();

    if(KWMScreen.MarkedWindow.WID != 0 &&
       KWMScreen.MarkedWindow.WID != -1)
    {
        KWMFocus.MarkedApplication = AXUIElementCreateApplication(KWMScreen.MarkedWindow.PID);
        if(!KWMFocus.MarkedApplication)
            return;

        AXError Error = AXObserverCreate(KWMScreen.MarkedWindow.PID, MarkedAXObserverCallback, &KWMFocus.MarkedObserver);
        if(Error == kAXErrorSuccess)
        {
            AXObserverAddNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXWindowMiniaturizedNotification, NULL);
            AXObserverAddNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXWindowMovedNotification, NULL);
            AXObserverAddNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXWindowResizedNotification, NULL);
            AXObserverAddNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXUIElementDestroyedNotification, NULL);
            CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(KWMFocus.MarkedObserver), kCFRunLoopDefaultMode);
        }
    }
}

void DestroyMarkedWindowNotifications()
{
    if(!KWMFocus.MarkedObserver)
        return;

    AXObserverRemoveNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXWindowMiniaturizedNotification);
    AXObserverRemoveNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXWindowMovedNotification);
    AXObserverRemoveNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXWindowResizedNotification);
    AXObserverRemoveNotification(KWMFocus.MarkedObserver, KWMFocus.MarkedApplication, kAXUIElementDestroyedNotification);
    CFRunLoopRemoveSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(KWMFocus.MarkedObserver), kCFRunLoopDefaultMode);

    CFRelease(KWMFocus.MarkedObserver);
    KWMFocus.MarkedObserver = NULL;
    CFRelease(KWMFocus.MarkedApplication);
    KWMFocus.MarkedApplication = NULL;
}

void CreateFocusedWindowNotifications()
{
    DestroyFocusedWindowNotifications();

    if(KWMFocus.Window)
    {
        KWMFocus.FocusedApplication = AXUIElementCreateApplication(KWMFocus.Window->PID);
        if(!KWMFocus.FocusedApplication)
            return;

        AXError Error = AXObserverCreate(KWMFocus.Window->PID, FocusedAXObserverCallback, &KWMFocus.FocusedObserver);
        if(Error == kAXErrorSuccess)
        {
            AXObserverAddNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXWindowMiniaturizedNotification, NULL);
            AXObserverAddNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXWindowMovedNotification, NULL);
            AXObserverAddNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXWindowResizedNotification, NULL);
            AXObserverAddNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXTitleChangedNotification, NULL);
            AXObserverAddNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXFocusedWindowChangedNotification, NULL);
            AXObserverAddNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXUIElementDestroyedNotification, NULL);
            CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(KWMFocus.FocusedObserver), kCFRunLoopDefaultMode);
        }
    }
}

void DestroyFocusedWindowNotifications()
{
    if(!KWMFocus.FocusedObserver)
        return;

    AXObserverRemoveNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXWindowMiniaturizedNotification);
    AXObserverRemoveNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXWindowMovedNotification);
    AXObserverRemoveNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXWindowResizedNotification);
    AXObserverRemoveNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXTitleChangedNotification);
    AXObserverRemoveNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXFocusedWindowChangedNotification);
    AXObserverRemoveNotification(KWMFocus.FocusedObserver, KWMFocus.FocusedApplication, kAXUIElementDestroyedNotification);
    CFRunLoopRemoveSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(KWMFocus.FocusedObserver), kCFRunLoopDefaultMode);

    CFRelease(KWMFocus.FocusedObserver);
    KWMFocus.FocusedObserver = NULL;
    CFRelease(KWMFocus.FocusedApplication);
    KWMFocus.FocusedApplication = NULL;
}

