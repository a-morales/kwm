#include "application.h"
#include "element.h"
#include "sharedworkspace.h"
#include "event.h"
#include "display.h"
#include "axlib.h"

#define internal static
#define local_persist static

enum ax_application_notifications
{
    AXApplication_Notification_WindowCreated,
    AXApplication_Notification_WindowFocused,
    AXApplication_Notification_WindowMoved,
    AXApplication_Notification_WindowResized,
    AXApplication_Notification_WindowTitle,

    AXApplication_Notification_Count
};

internal CFStringRef
AXNotificationFromEnum(int Type)
{
    switch(Type)
    {
        case AXApplication_Notification_WindowCreated:
        {
            return kAXWindowCreatedNotification;
        } break;
        case AXApplication_Notification_WindowFocused:
        {
            return kAXFocusedWindowChangedNotification;
        } break;
        case AXApplication_Notification_WindowMoved:
        {
            return kAXWindowMovedNotification;
        } break;
        case AXApplication_Notification_WindowResized:
        {
            return kAXWindowResizedNotification;
        } break;
        case AXApplication_Notification_WindowTitle:
        {
            return kAXTitleChangedNotification;
        } break;
        default: { return NULL; /* NOTE(koekeishiya): Should never happen */ } break;
    }
}

internal inline ax_window *
AXLibGetWindowByRef(ax_application *Application, AXUIElementRef WindowRef)
{
    uint32_t WID = AXLibGetWindowID(WindowRef);
    return AXLibFindApplicationWindow(Application, WID);
}

internal inline void
AXLibUpdateApplicationWindowTitle(ax_application *Application, AXUIElementRef WindowRef)
{
    ax_window *Window = AXLibGetWindowByRef(Application, WindowRef);
    if(Window)
        Window->Name = AXLibGetWindowTitle(WindowRef);
}

internal inline void
AXLibDestroyInvalidWindow(ax_window *Window)
{
    AXLibRemoveObserverNotification(&Window->Application->Observer, Window->Ref, kAXUIElementDestroyedNotification);
    AXLibDestroyWindow(Window);
}

OBSERVER_CALLBACK(AXApplicationCallback)
{
    ax_application *Application = (ax_application *) Reference;

    if(CFEqual(Notification, kAXWindowCreatedNotification))
    {
        /* NOTE(koekeishiya): Construct ax_window struct for the newly created window */
        ax_window *Window = AXLibConstructWindow(Application, Element);
        if(AXLibAddObserverNotification(&Application->Observer, Window->Ref, kAXUIElementDestroyedNotification, Window) == kAXErrorSuccess)
        {
            AXLibAddApplicationWindow(Application, Window);

            /* NOTE(koekeishiya): Triggers an AXEvent_WindowCreated and passes a pointer to the new ax_window */
            AXLibConstructEvent(AXEvent_WindowCreated, Window, false);

            /* NOTE(koekeishiya): When a new window is created, we incorrectly receive the kAXFocusedWindowChangedNotification
                                  first, for some reason. We discard that notification and restore it when we have the window to work with. */
            AXApplicationCallback(Observer, Window->Ref, kAXFocusedWindowChangedNotification, Application);
        }
        else
        {
            /* NOTE(koekeishiya): This element is not destructible and cannot be an application window (?) */
            AXLibDestroyInvalidWindow(Window);
        }
    }
    else if(CFEqual(Notification, kAXUIElementDestroyedNotification))
    {
        /* NOTE(koekeishiya): If the destroyed UIElement is a window, remove it from the list. */
        ax_window *Window = (ax_window *) Reference;
        if(Window)
        {
            AXLibRemoveApplicationWindow(Window->Application, Window->ID);
            Window->Application->Focus = AXLibGetFocusedWindow(Window->Application);

            /* NOTE(koekeishiya): The callback is responsible for calling AXLibDestroyWindow(Window); */
            AXLibConstructEvent(AXEvent_WindowDestroyed, Window, false);
        }
    }
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
        /* NOTE(koekeishiya): This notification could be received before the window itself is created.
                              Make sure that the window actually exists before we notify our callback. */
        ax_window *Window = AXLibGetWindowByRef(Application, Element);
        if(Window)
        {
            /* NOTE(koekeishiya): When a window is deminimized, we receive a FocusedWindowChanged notification before the
               window is visible. Only notify our callback when we know that we can interact with the window in question. */
            if(!AXLibHasFlags(Window, AXWindow_Minimized))
            {
                AXLibConstructEvent(AXEvent_WindowFocused, Window, false);
            }

            /* NOTE(koekeishiya): If the application corresponding to this window is flagged for activation and
                                  the window is visible to the user, this should be the focused application. */
            if(AXLibHasFlags(Window->Application, AXApplication_Activate))
            {
                AXLibClearFlags(Window->Application, AXApplication_Activate);
                if(!AXLibHasFlags(Window, AXWindow_Minimized))
                    AXLibConstructEvent(AXEvent_ApplicationActivated, Window->Application, false);
            }
        }
    }
    else if(CFEqual(Notification, kAXWindowMiniaturizedNotification))
    {
        /* NOTE(koekeishiya): Triggers an AXEvent_WindowMinimized and passes a pointer to the ax_window */
        ax_window *Window = (ax_window *) Reference;
        if(Window)
        {
            AXLibAddFlags(Window, AXWindow_Minimized);
            AXLibConstructEvent(AXEvent_WindowMinimized, Window, false);
        }
    }
    else if(CFEqual(Notification, kAXWindowDeminiaturizedNotification))
    {
        /* NOTE(koekeishiya): Triggers an AXEvent_WindowDeminimized and passes a pointer to the ax_window */
        ax_window *Window = (ax_window *) Reference;
        if(Window)
        {
            /* NOTE(koekeishiya): If a window was minimized before AXLib is initialized, the WindowID is
                                  reported as 0. We check if this window is one of these, update the
                                  WindowID and place it in our managed window list. */
            if(Window->ID == 0)
            {
                for(int Index = 0; Index < Window->Application->NullWindows.size(); ++Index)
                {
                    if(Window->Application->NullWindows[Index] == Window)
                        Window->Application->NullWindows.erase(Window->Application->NullWindows.begin() + Index);
                }

                Window->ID = AXLibGetWindowID(Window->Ref);
                Window->Application->Windows[Window->ID] = Window;
            }

            /* NOTE(koekeishiya): kAXWindowDeminiaturized is sent before didActiveSpaceChange, when a deminimized
                                  window pulls you to the space of that window. If the active space of the display
                                  is not equal to the space of the window, we should ignore this event and schedule
                                  a new one to happen after the next space changed event. */

            ax_display *Display = AXLibWindowDisplay(Window);
            if(AXLibSpaceHasWindow(Window, Display->Space->ID))
            {
                AXLibConstructEvent(AXEvent_WindowDeminimized, Window, false);
            }
            else
            {
                AXLibAddFlags(Display->Space, AXSpace_DeminimizedTransition);
            }
        }
    }
    else if(CFEqual(Notification, kAXWindowMovedNotification))
    {
        /* NOTE(koekeishiya): Triggers an AXEvent_WindowMoved and passes a pointer to the ax_window */
        ax_window *Window = AXLibGetWindowByRef(Application, Element);
        if(Window)
        {
            Window->Position = AXLibGetWindowPosition(Window->Ref);

            AXLibConstructEvent(AXEvent_WindowMoved, Window, AXLibHasFlags(Window, AXWindow_MoveIntrinsic));
            AXLibClearFlags(Window, AXWindow_MoveIntrinsic);
        }
    }
    else if(CFEqual(Notification, kAXWindowResizedNotification))
    {
        /* NOTE(koekeishiya): Triggers an AXEvent_WindowResized and passes a pointer to the ax_window */
        ax_window *Window = AXLibGetWindowByRef(Application, Element);
        if(Window)
        {
            Window->Position = AXLibGetWindowPosition(Window->Ref);
            Window->Size = AXLibGetWindowSize(Window->Ref);

            AXLibConstructEvent(AXEvent_WindowResized, Window, AXLibHasFlags(Window, AXWindow_SizeIntrinsic));
            AXLibClearFlags(Window, AXWindow_SizeIntrinsic);
        }
    }
    else if(CFEqual(Notification, kAXTitleChangedNotification))
    {
        AXLibUpdateApplicationWindowTitle(Application, Element);
    }
}

ax_application AXLibConstructApplication(pid_t PID, std::string Name)
{
    ax_application Application = {};

    Application.Ref = AXUIElementCreateApplication(PID);
    GetProcessForPID(PID, &Application.PSN);
    Application.Name = Name;
    Application.PID = PID;

    return Application;
}

void AXLibInitializeApplication(ax_application *Application)
{
    AXLibAddApplicationObserver(Application);
    AXLibAddApplicationWindows(Application);
    Application->Focus = AXLibGetFocusedWindow(Application);
}

/* NOTE(koekeishiya): This function will try to reinitialize the application that failed during creation. */
void AXLibAddApplicationObserverNotificationFallback(ax_application *Application)
{
    AXLibRemoveApplicationObserver(Application);
    AXLibInitializeApplication(Application);
    AXLibConstructEvent(AXEvent_ApplicationLaunched, Application, false);
}

bool AXLibHasApplicationObserverNotification(ax_application *Application)
{
    for(int Notification = AXApplication_Notification_WindowCreated;
            Notification < AXApplication_Notification_Count;
            ++Notification)
    {
        if(!(Application->Notifications & (1 << Notification)))
        {
            return false;
        }
    }

    return true;
}

void AXLibAddApplicationObserver(ax_application *Application)
{
    AXLibConstructObserver(Application, AXApplicationCallback);
    if(Application->Observer.Valid)
    {
        for(int Notification = AXApplication_Notification_WindowCreated;
                Notification < AXApplication_Notification_Count;
                ++Notification)
        {
            int Attempts = 10;
            while((--Attempts > 0) &&
                  (AXLibAddObserverNotification(&Application->Observer, Application->Ref, AXNotificationFromEnum(Notification), Application) != kAXErrorSuccess))
            {
                /* NOTE(koekeishiya): Could not add notification because the application has not finishied initializing yet.
                                      Sleep for a short while and try again. We limit the number of tries to prevent a deadlock. */
                usleep(10000);
            }

            /* NOTE(koekeishiya): Mark the notification as successful. */
            if(Attempts != 0)
            {
                Application->Notifications |= (1 << Notification);
            }
        }

        AXLibStartObserver(&Application->Observer);
    }
}

void AXLibRemoveApplicationObserver(ax_application *Application)
{
    if(Application->Observer.Valid)
    {
        AXLibStopObserver(&Application->Observer);

        for(int Notification = AXApplication_Notification_WindowCreated;
                Notification < AXApplication_Notification_Count;
                ++Notification)
        {
            AXLibRemoveObserverNotification(&Application->Observer, Application->Ref, AXNotificationFromEnum(Notification));
        }

        AXLibDestroyObserver(&Application->Observer);
    }
}

void AXLibAddApplicationWindows(ax_application *Application)
{
    CFArrayRef Windows = (CFArrayRef) AXLibGetWindowProperty(Application->Ref, kAXWindowsAttribute);
    if(Windows)
    {
        CFIndex Count = CFArrayGetCount(Windows);
        for(CFIndex Index = 0; Index < Count; ++Index)
        {
            AXUIElementRef Ref = (AXUIElementRef) CFArrayGetValueAtIndex(Windows, Index);
            if(!AXLibGetWindowByRef(Application, Ref))
            {
                ax_window *Window = AXLibConstructWindow(Application, Ref);
                if(AXLibAddObserverNotification(&Application->Observer, Window->Ref, kAXUIElementDestroyedNotification, Window) == kAXErrorSuccess)
                {
                    AXLibAddApplicationWindow(Application, Window);
                }
                else
                {
                    AXLibDestroyWindow(Window);
                }
            }
        }
        CFRelease(Windows);
    }
}

void AXLibRemoveApplicationWindows(ax_application *Application)
{
    std::map<uint32_t, ax_window*>::iterator It;
    for(It = Application->Windows.begin(); It != Application->Windows.end(); ++It)
    {
        ax_window *Window = It->second;
        AXLibRemoveObserverNotification(&Window->Application->Observer, Window->Ref, kAXUIElementDestroyedNotification);
        AXLibRemoveObserverNotification(&Window->Application->Observer, Window->Ref, kAXWindowMiniaturizedNotification);
        AXLibRemoveObserverNotification(&Window->Application->Observer, Window->Ref, kAXWindowDeminiaturizedNotification);
        AXLibDestroyWindow(Window);
    }

    Application->Windows.clear();
}

ax_window *AXLibFindApplicationWindow(ax_application *Application, uint32_t WID)
{
    std::map<uint32_t, ax_window*>::iterator It;
    It = Application->Windows.find(WID);
    if(It != Application->Windows.end())
        return It->second;
    else
        return NULL;
}

void AXLibAddApplicationWindow(ax_application *Application, ax_window *Window)
{
    if(!AXLibFindApplicationWindow(Application, Window->ID))
    {
        AXLibAddObserverNotification(&Application->Observer, Window->Ref, kAXWindowMiniaturizedNotification, Window);
        AXLibAddObserverNotification(&Application->Observer, Window->Ref, kAXWindowDeminiaturizedNotification, Window);

        if(Window->ID == 0)
            Application->NullWindows.push_back(Window);
        else
            Application->Windows[Window->ID] = Window;
    }
}

void AXLibRemoveApplicationWindow(ax_application *Application, uint32_t WID)
{
    ax_window *Window = AXLibFindApplicationWindow(Application, WID);
    if(Window)
    {
        AXLibRemoveObserverNotification(&Window->Application->Observer, Window->Ref, kAXUIElementDestroyedNotification);
        AXLibRemoveObserverNotification(&Window->Application->Observer, Window->Ref, kAXWindowMiniaturizedNotification);
        AXLibRemoveObserverNotification(&Window->Application->Observer, Window->Ref, kAXWindowDeminiaturizedNotification);
        Application->Windows.erase(WID);
    }
}

void AXLibActivateApplication(ax_application *Application)
{
    SharedWorkspaceActivateApplication(Application->PID);
}

bool AXLibIsApplicationActive(ax_application *Application)
{
    return SharedWorkspaceIsApplicationActive(Application->PID);
}

bool AXLibIsApplicationHidden(ax_application *Application)
{
    return SharedWorkspaceIsApplicationHidden(Application->PID);
}

void AXLibDestroyApplication(ax_application *Application)
{
    AXLibRemoveApplicationWindows(Application);
    AXLibRemoveApplicationObserver(Application);
    CFRelease(Application->Ref);
    Application->Ref = NULL;
}
