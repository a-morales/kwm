#include "application.h"
#include "element.h"
#include "sharedworkspace.h"
#include "event.h"
#include "axlib.h"

#define internal static
#define local_persist static

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

OBSERVER_CALLBACK(AXApplicationCallback)
{
    ax_application *Application = (ax_application*)Reference;

    if(CFEqual(Notification, kAXWindowCreatedNotification))
    {
        // printf("%s: kAXWindowCreatedNotification\n", Application->Name.c_str());

        /* NOTE(koekeishiya): Construct ax_window struct for the newly created window */
        ax_window Window = AXLibConstructWindow(Application, Element);
        AXLibAddApplicationWindow(Application, Window);

        /* NOTE(koekeishiya): Triggers an AXEvent_WindowCreated and passes a pointer to the new ax_window */
        ax_window *WindowPtr = AXLibFindApplicationWindow(Application, Window.ID);
        AXLibConstructEvent(AXEvent_WindowCreated, WindowPtr);

        /* NOTE(koekeishiya): When a new window is created, we incorrectly receive the kAXFocusedWindowChangedNotification
                              first, for some reason. We discard that notification and restore it when we have the window to work with. */
        Application->Focus = WindowPtr;
        AXLibConstructEvent(AXEvent_WindowFocused, WindowPtr);
    }
    else if(CFEqual(Notification, kAXUIElementDestroyedNotification))
    {
        printf("%s: kAXUIElementDestroyedNotification\n", Application->Name.c_str());

        /* NOTE(koekeishiya): If the destroyed UIElement is a window, remove it from the application window list. */
        /* TODO(koekeishiya): Why does WID equal 0, even for AXUIElementRefs that are a valid window (?) */
        uint32_t WID = AXLibGetWindowID(Element);
        AXLibRemoveApplicationWindow(Application, WID);
    }
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
        // printf("%s: kAXFocusedWindowChangedNotification\n", Application->Name.c_str());

        /* NOTE(koekeishiya): This notification could be received before the window itself is created.
                              Make sure that the window actually exists before we notify our callback. */
        ax_window *Window = AXLibGetWindowByRef(Application, Element);
        if(Window)
        {
            Application->Focus = Window;
            AXLibConstructEvent(AXEvent_WindowFocused, Window);
        }
    }
    else if(CFEqual(Notification, kAXWindowMiniaturizedNotification))
    {
        // printf("%s: kAXWindowMiniaturizedNotification\n", Application->Name.c_str());
        /* TODO(koekeishiya): NYI */
    }
    else if(CFEqual(Notification, kAXWindowDeminiaturizedNotification))
    {
        // printf("%s: kAXWindowDeminiaturizedNotification\n", Application->Name.c_str());
        /* TODO(koekeishiya): NYI */
    }
    else if(CFEqual(Notification, kAXWindowMovedNotification))
    {
        // printf("%s: kAXWindowMovedNotification\n", Application->Name.c_str());

        /* NOTE(koekeishiya): Triggers an AXEvent_WindowMoved and passes a pointer to the ax_window */
        ax_window *Window = AXLibGetWindowByRef(Application, Element);
        AXLibConstructEvent(AXEvent_WindowMoved, Window);
    }
    else if(CFEqual(Notification, kAXWindowResizedNotification))
    {
        // printf("%s: kAXWindowResizedNotification\n", Application->Name.c_str());

        /* NOTE(koekeishiya): Triggers an AXEvent_WindowResized and passes a pointer to the ax_window */
        ax_window *Window = AXLibGetWindowByRef(Application, Element);
        AXLibConstructEvent(AXEvent_WindowResized, Window);
    }
    else if(CFEqual(Notification, kAXTitleChangedNotification))
    {
        // printf("%s: kAXTitleChangedNotification\n", Application->Name.c_str());
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

void AXLibAddApplicationObserver(ax_application *Application)
{
    AXLibConstructObserver(Application, AXApplicationCallback);
    if(Application->Observer.Valid)
    {
        printf("AXLIB Create observer: %s\n", Application->Name.c_str());
        AXLibAddObserverNotification(&Application->Observer, kAXWindowCreatedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXFocusedWindowChangedNotification, Application);

        AXLibAddObserverNotification(&Application->Observer, kAXWindowMiniaturizedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXWindowDeminiaturizedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXWindowMovedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXWindowResizedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXTitleChangedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXUIElementDestroyedNotification, Application);

        AXLibStartObserver(&Application->Observer);
    }
}

void AXLibRemoveApplicationObserver(ax_application *Application)
{
    if(Application->Observer.Valid)
    {
        AXLibStopObserver(&Application->Observer);

        AXLibRemoveObserverNotification(&Application->Observer, kAXWindowCreatedNotification);
        AXLibRemoveObserverNotification(&Application->Observer, kAXFocusedWindowChangedNotification);

        AXLibRemoveObserverNotification(&Application->Observer, kAXWindowMiniaturizedNotification);
        AXLibRemoveObserverNotification(&Application->Observer, kAXWindowDeminiaturizedNotification);
        AXLibRemoveObserverNotification(&Application->Observer, kAXWindowMovedNotification);
        AXLibRemoveObserverNotification(&Application->Observer, kAXWindowResizedNotification);
        AXLibRemoveObserverNotification(&Application->Observer, kAXTitleChangedNotification);
        AXLibRemoveObserverNotification(&Application->Observer, kAXUIElementDestroyedNotification);

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
            AXLibAddApplicationWindow(Application, AXLibConstructWindow(Application, Ref));
        }
    }
}

void AXLibRemoveApplicationWindows(ax_application *Application)
{
    std::map<uint32_t, ax_window>::iterator It;
    for(It = Application->Windows.begin(); It != Application->Windows.end(); ++It)
        AXLibDestroyWindow(&It->second);

    Application->Windows.clear();
}

ax_window *AXLibFindApplicationWindow(ax_application *Application, uint32_t WID)
{
    std::map<uint32_t, ax_window>::iterator It;
    It = Application->Windows.find(WID);
    if(It != Application->Windows.end())
        return &It->second;
    else
        return NULL;
}

void AXLibAddApplicationWindow(ax_application *Application, ax_window Window)
{
    if(!AXLibFindApplicationWindow(Application, Window.ID))
        Application->Windows[Window.ID] = Window;
}

void AXLibRemoveApplicationWindow(ax_application *Application, uint32_t WID)
{
    ax_window *Window = AXLibFindApplicationWindow(Application, WID);
    if(Window)
    {
        AXLibDestroyWindow(Window);
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

void AXLibDestroyApplication(ax_application *Application)
{
    AXLibRemoveApplicationObserver(Application);
    AXLibRemoveApplicationWindows(Application);
    CFRelease(Application->Ref);
    Application->Ref = NULL;
}
