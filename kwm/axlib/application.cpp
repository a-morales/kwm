#include "application.h"
#include "element.h"
#include "sharedworkspace.h"
#include "event.h"

#define internal static
#define local_persist static

internal void
AXLibUpdateApplicationFocus(ax_application *Application, AXUIElementRef WindowRef)
{
    uint32_t WID = AXLibGetWindowID(WindowRef);
    ax_window *Window = AXLibFindApplicationWindow(Application, WID);
    if(Window)
        Application->Focus = Window;
}

internal void
AXLibUpdateApplicationWindowTitle(ax_application *Application, AXUIElementRef WindowRef)
{
    uint32_t WID = AXLibGetWindowID(WindowRef);
    ax_window *Window = AXLibFindApplicationWindow(Application, WID);
    if(Window)
        Window->Name = AXLibGetWindowTitle(WindowRef);
}

OBSERVER_CALLBACK(AXApplicationCallback)
{
    ax_application *Application = (ax_application*)Reference;

    if(CFEqual(Notification, kAXWindowCreatedNotification))
    {
        printf("%s: kAXWindowCreatedNotification\n", Application->Name.c_str());
        AXLibAddApplicationWindow(Application, AXLibConstructWindow(Application, Element));
    }
    else if(CFEqual(Notification, kAXUIElementDestroyedNotification))
    {
        printf("%s: kAXUIElementDestroyedNotification\n", Application->Name.c_str());
        uint32_t WID = AXLibGetWindowID(Element);
        AXLibRemoveApplicationWindow(Application, WID);
    }
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
        printf("%s: kAXFocusedWindowChangedNotification\n", Application->Name.c_str());
        AXLibUpdateApplicationFocus(Application, Element);
        AXLibConstructEvent(AXEvent_WindowFocused, NULL);
    }
    else if(CFEqual(Notification, kAXWindowMiniaturizedNotification))
    {
        printf("%s: kAXWindowMiniaturizedNotification\n", Application->Name.c_str());
        /* TODO(koekeishiya): NYI */
    }
    else if(CFEqual(Notification, kAXWindowDeminiaturizedNotification))
    {
        printf("%s: kAXWindowDeminiaturizedNotification\n", Application->Name.c_str());
        /* TODO(koekeishiya): NYI */
    }
    else if(CFEqual(Notification, kAXWindowMovedNotification))
    {
        printf("%s: kAXWindowMovedNotification\n", Application->Name.c_str());
        AXLibConstructEvent(AXEvent_WindowMoved, NULL);
        /* TODO(koekeishiya): NYI */
    }
    else if(CFEqual(Notification, kAXWindowResizedNotification))
    {
        printf("%s: kAXWindowResizedNotification\n", Application->Name.c_str());
        AXLibConstructEvent(AXEvent_WindowResized, NULL);
        /* TODO(koekeishiya): NYI */
    }
    else if(CFEqual(Notification, kAXTitleChangedNotification))
    {
        printf("%s: kAXTitleChangedNotification\n", Application->Name.c_str());
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
