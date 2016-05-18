#include "notifications.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "border.h"
#include "axlib/element.h"
#include "axlib/observer.h"

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
        Window->Name = AXLibGetWindowTitle(Element);
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
        if(!Window || Window->WID != AXLibGetWindowID(Element))
        {
            window_info *OSXWindow = GetWindowByID(AXLibGetWindowID(Element));
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
    else if(Window &&
           (CFEqual(Notification, kAXWindowResizedNotification) ||
            CFEqual(Notification, kAXWindowMovedNotification)))
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
        KWMFocus.MarkedObserver = AXLibConstructObserver(KWMScreen.MarkedWindow.PID, MarkedAXObserverCallback);
        if(KWMFocus.MarkedObserver.Ref && KWMFocus.MarkedObserver.AppRef)
        {
            AXLibAddObserverNotification(&KWMFocus.MarkedObserver, kAXWindowMiniaturizedNotification);
            AXLibAddObserverNotification(&KWMFocus.MarkedObserver, kAXWindowMovedNotification);
            AXLibAddObserverNotification(&KWMFocus.MarkedObserver, kAXWindowResizedNotification);
            AXLibAddObserverNotification(&KWMFocus.MarkedObserver, kAXUIElementDestroyedNotification);
            AXLibStartObserver(&KWMFocus.MarkedObserver);
        }

    }
}

void DestroyMarkedWindowNotifications()
{
    if(KWMFocus.MarkedObserver.Ref && KWMFocus.MarkedObserver.AppRef)
    {
        AXLibStopObserver(&KWMFocus.MarkedObserver);
        AXLibRemoveObserverNotification(&KWMFocus.MarkedObserver, kAXWindowMiniaturizedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.MarkedObserver, kAXWindowMovedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.MarkedObserver, kAXWindowResizedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.MarkedObserver, kAXUIElementDestroyedNotification);
        AXLibDestroyObserver(&KWMFocus.MarkedObserver);
    }
}

void CreateFocusedWindowNotifications()
{
    DestroyFocusedWindowNotifications();

    if(KWMFocus.Window)
    {
        KWMFocus.FocusedObserver = AXLibConstructObserver(KWMFocus.Window->PID, FocusedAXObserverCallback);
        if(KWMFocus.FocusedObserver.Ref && KWMFocus.FocusedObserver.AppRef)
        {
            AXLibAddObserverNotification(&KWMFocus.FocusedObserver, kAXWindowMiniaturizedNotification);
            AXLibAddObserverNotification(&KWMFocus.FocusedObserver, kAXWindowMovedNotification);
            AXLibAddObserverNotification(&KWMFocus.FocusedObserver, kAXWindowResizedNotification);
            AXLibAddObserverNotification(&KWMFocus.FocusedObserver, kAXTitleChangedNotification);
            AXLibAddObserverNotification(&KWMFocus.FocusedObserver, kAXFocusedWindowChangedNotification);
            AXLibAddObserverNotification(&KWMFocus.FocusedObserver, kAXUIElementDestroyedNotification);
            AXLibStartObserver(&KWMFocus.FocusedObserver);
        }
    }
}

void DestroyFocusedWindowNotifications()
{
    if(KWMFocus.FocusedObserver.Ref && KWMFocus.FocusedObserver.AppRef)
    {
        AXLibStopObserver(&KWMFocus.FocusedObserver);
        AXLibRemoveObserverNotification(&KWMFocus.FocusedObserver, kAXWindowMiniaturizedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.FocusedObserver, kAXWindowMovedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.FocusedObserver, kAXWindowResizedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.FocusedObserver, kAXTitleChangedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.FocusedObserver, kAXFocusedWindowChangedNotification);
        AXLibRemoveObserverNotification(&KWMFocus.FocusedObserver, kAXUIElementDestroyedNotification);
        AXLibDestroyObserver(&KWMFocus.FocusedObserver);
    }
}
