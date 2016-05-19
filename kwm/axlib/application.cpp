#include "application.h"
#include "element.h"
#include "sharedworkspace.h"

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
            Application->Windows.push_back(AXLibConstructWindow(Ref));
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
    CFRelease(Application->Ref);
    Application->Ref = NULL;
    AXLibRemoveApplicationWindows(Application);
}
