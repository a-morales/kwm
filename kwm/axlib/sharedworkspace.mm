#import <Cocoa/Cocoa.h>
#import "sharedworkspace.h"

#define internal static
internal std::map<pid_t, ax_application> *Applications;

void SharedWorkspaceSetApplicationsPointer(std::map<pid_t, ax_application> *Apps)
{
    Applications = Apps;
}

std::vector<pid_t> SharedWorkspaceRunningApplications()
{
    std::vector<pid_t> List;

    for(NSRunningApplication *Application in [[NSWorkspace sharedWorkspace] runningApplications])
        List.push_back(Application.processIdentifier);

    return List;
}

void SharedWorkspaceDidLaunchApplication(pid_t PID)
{
    (*Applications)[PID] = AXLibConstructApplication(PID);
}

void SharedWorkspaceDidTerminateApplication(pid_t PID)
{
    std::map<pid_t, ax_application>::iterator It = Applications->find(PID);
    if(It != Applications->end())
    {
        AXLibDestroyApplication(&It->second);
        Applications->erase(PID);
    }
}
