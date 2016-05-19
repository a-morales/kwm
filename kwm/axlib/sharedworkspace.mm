#import <Cocoa/Cocoa.h>
#import "sharedworkspace.h"

std::vector<pid_t> SharedWorkspaceRunningApplications()
{
    std::vector<pid_t> List;

    for(NSRunningApplication *Application in [[NSWorkspace sharedWorkspace] runningApplications])
        List.push_back(Application.processIdentifier);

    return List;
}
