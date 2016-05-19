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
            AXLibAddObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXWindowMiniaturizedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXWindowMovedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXWindowResizedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXUIElementDestroyedNotification, NULL);
            AXLibStartObserver(&KWMFocus.MarkedObserver);
        }

    }
}

void DestroyMarkedWindowNotifications()
{
    if(KWMFocus.MarkedObserver.Ref && KWMFocus.MarkedObserver.AppRef)
    {
        AXLibStopObserver(&KWMFocus.MarkedObserver);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXWindowMiniaturizedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXWindowMovedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXWindowResizedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.MarkedObserver, kAXUIElementDestroyedNotification);
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
            AXLibAddObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXWindowMiniaturizedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXWindowMovedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXWindowResizedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXTitleChangedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXFocusedWindowChangedNotification, NULL);
            AXLibAddObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXUIElementDestroyedNotification, NULL);
            AXLibStartObserver(&KWMFocus.FocusedObserver);
        }
    }
}

void DestroyFocusedWindowNotifications()
{
    if(KWMFocus.FocusedObserver.Ref && KWMFocus.FocusedObserver.AppRef)
    {
        AXLibStopObserver(&KWMFocus.FocusedObserver);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXWindowMiniaturizedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXWindowMovedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXWindowResizedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXTitleChangedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXFocusedWindowChangedNotification);
        AXLibRemoveObserverNotificationOLD(&KWMFocus.FocusedObserver, kAXUIElementDestroyedNotification);
        AXLibDestroyObserver(&KWMFocus.FocusedObserver);
    }
}
