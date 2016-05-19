#include "application.h"
#include "element.h"
#include "sharedworkspace.h"

OBSERVER_CALLBACK(AXApplicationCallback)
{
    ax_application *Application = (ax_application*)Reference;

    if(CFEqual(Notification, kAXWindowCreatedNotification))
    {
        printf("%s: kAXWindowCreatedNotification\n", Application->Name.c_str());
        ax_window Window = AXLibConstructWindow(Application, Element);
        Application->Windows[Window.ID] = Window;
    }
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
        printf("%s: kAXFocusedWindowChangedNotification\n", Application->Name.c_str());
    }
    else if(CFEqual(Notification, kAXWindowMiniaturizedNotification))
    {
        printf("%s: kAXWindowMiniaturizedNotification\n", Application->Name.c_str());
    }
    else if(CFEqual(Notification, kAXWindowMovedNotification))
    {
        printf("%s: kAXWindowMovedNotification\n", Application->Name.c_str());
    }
    else if(CFEqual(Notification, kAXWindowResizedNotification))
    {
        printf("%s: kAXWindowResizedNotification\n", Application->Name.c_str());
    }
    else if(CFEqual(Notification, kAXTitleChangedNotification))
    {
        printf("%s: kAXTitleChangedNotification\n", Application->Name.c_str());
        int ID = AXLibGetWindowID(Element);
        ax_window *Window = AXLibFindApplicationWindow(Application, ID);
        if(Window)
            Window->Name = AXLibGetWindowTitle(Element);
    }
    else if(CFEqual(Notification, kAXUIElementDestroyedNotification))
    {
        printf("%s: kAXUIElementDestroyedNotification\n", Application->Name.c_str());
        int ID = AXLibGetWindowID(Element);
        ax_window *Window = AXLibFindApplicationWindow(Application, ID);
        if(Window)
        {
            AXLibDestroyWindow(Window);
            Application->Windows.erase(ID);
        }
    }
}

void AXLibRunningApplications(std::map<pid_t, ax_application> *AXApplications)
{
    std::map<pid_t, std::string> List = SharedWorkspaceRunningApplications();

    std::map<pid_t, std::string>::iterator It;
    for(It = List.begin(); It != List.end(); ++It)
    {
        pid_t PID = It->first;
        std::string Name = It->second;
        if(Name == "kwm-overlay") continue;
        (*AXApplications)[PID] = AXLibConstructApplication(PID, Name);
        AXLibAddApplicationWindows(&(*AXApplications)[PID]);
        AXLibAddApplicationObserver(&(*AXApplications)[PID]);
    }
}

ax_application AXLibConstructApplication(int PID, std::string Name)
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
    if(Application->Observer.Ref)
    {
        printf("AXLIB Create observer: %s\n", Application->Name.c_str());
        AXLibAddObserverNotification(&Application->Observer, kAXWindowCreatedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXFocusedWindowChangedNotification, Application);

        AXLibAddObserverNotification(&Application->Observer, kAXWindowMiniaturizedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXWindowMovedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXWindowResizedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXTitleChangedNotification, Application);
        AXLibAddObserverNotification(&Application->Observer, kAXUIElementDestroyedNotification, Application);

        AXLibStartObserver(&Application->Observer);
    }
}

void AXLibRemoveApplicationObserver(ax_application *Application)
{
    AXLibStopObserver(&Application->Observer);

    AXLibRemoveObserverNotification(&Application->Observer, kAXWindowCreatedNotification);
    AXLibRemoveObserverNotification(&Application->Observer, kAXFocusedWindowChangedNotification);

    AXLibRemoveObserverNotification(&Application->Observer, kAXWindowMiniaturizedNotification);
    AXLibRemoveObserverNotification(&Application->Observer, kAXWindowMovedNotification);
    AXLibRemoveObserverNotification(&Application->Observer, kAXWindowResizedNotification);
    AXLibRemoveObserverNotification(&Application->Observer, kAXTitleChangedNotification);
    AXLibRemoveObserverNotification(&Application->Observer, kAXUIElementDestroyedNotification);

    AXLibDestroyObserver(&Application->Observer);
}

void AXLibAddApplicationWindows(ax_application *Application)
{
    CFArrayRef Windows = (CFArrayRef) AXLibGetWindowProperty(Application->Ref,
                                                             kAXWindowsAttribute);

    if(Windows)
    {
        CFIndex Count = CFArrayGetCount(Windows);
        for(CFIndex Index = 0; Index < Count; ++Index)
        {
            AXUIElementRef Ref = (AXUIElementRef) CFArrayGetValueAtIndex(Windows, Index);
            ax_window Window = AXLibConstructWindow(Application, Ref);
            Application->Windows[Window.ID] = Window;
        }
    }
}

void AXLibRemoveApplicationWindows(ax_application *Application)
{
    std::map<int, ax_window>::iterator It;
    for(It = Application->Windows.begin(); It != Application->Windows.end(); ++It)
        AXLibDestroyWindow(&It->second);

    Application->Windows.clear();
}

ax_window *AXLibFindApplicationWindow(ax_application *Application, int WID)
{
    std::map<int, ax_window>::iterator It;
    It = Application->Windows.find(WID);
    if(It != Application->Windows.end())
        return &It->second;
    else
        return NULL;
}

void AXLibDestroyApplication(ax_application *Application)
{
    if(Application->Observer.Ref)
        AXLibRemoveApplicationObserver(Application);

    AXLibRemoveApplicationWindows(Application);
    CFRelease(Application->Ref);
    Application->Ref = NULL;
}
