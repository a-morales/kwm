#include "application.h"
#include "element.h"
#include "sharedworkspace.h"

OBSERVER_CALLBACK(AXApplicationCallback)
{
    ax_application *Application = (ax_application*)Reference;

    if(CFEqual(Notification, kAXWindowCreatedNotification))
    {
        Application->Windows.push_back(AXLibConstructWindow(Application, Element));
    }
    else if(CFEqual(Notification, kAXFocusedWindowChangedNotification))
    {
    }
}

std::map<pid_t, ax_application> AXLibRunningApplications()
{
    std::vector<pid_t> List = SharedWorkspaceRunningApplications();
    std::map<pid_t, ax_application> Applications;

    for(int Index = 0; Index < List.size(); ++Index)
    {
        pid_t PID = List[Index];
        Applications[PID] = AXLibConstructApplication(PID);
    }

    return Applications;
}

ax_application AXLibConstructApplication(int PID)
{
    ax_application Application = {};

    Application.Ref = AXUIElementCreateApplication(PID);
    GetProcessForPID(PID, &Application.PSN);
    Application.PID = PID;

    return Application;
}

void AXLibAddApplicationObserver(ax_application *Application)
{
    AXLibConstructObserver(Application, AXApplicationCallback);

    AXLibAddObserverNotification(&Application->Observer, kAXWindowCreatedNotification, Application);
    AXLibAddObserverNotification(&Application->Observer, kAXFocusedWindowChangedNotification, Application);

    AXLibAddObserverNotification(&Application->Observer, kAXWindowMiniaturizedNotification, Application);
    AXLibAddObserverNotification(&Application->Observer, kAXWindowMovedNotification, Application);
    AXLibAddObserverNotification(&Application->Observer, kAXWindowResizedNotification, Application);
    AXLibAddObserverNotification(&Application->Observer, kAXTitleChangedNotification, Application);
    AXLibAddObserverNotification(&Application->Observer, kAXUIElementDestroyedNotification, Application);

    AXLibStartObserver(&Application->Observer);
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
            Application->Windows.push_back(AXLibConstructWindow(Application, Ref));
        }
    }
}

void AXLibRemoveApplicationWindows(ax_application *Application)
{
    for(int Index = 0; Index < Application->Windows.size(); ++Index)
        AXLibDestroyWindow(&Application->Windows[Index]);

    Application->Windows.clear();
}

void AXLibDestroyApplication(ax_application *Application)
{
    AXLibRemoveApplicationObserver(Application);
    AXLibRemoveApplicationWindows(Application);

    CFRelease(Application->Ref);
    Application->Ref = NULL;
}
