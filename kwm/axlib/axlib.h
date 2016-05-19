#ifndef AXLIB_MAIN_H
#define AXLIB_MAIN_H

#include "types.h"
#include "observer.h"
#include "element.h"
#include "window.h"
#include "application.h"
#include "sharedworkspace.h"

inline void
AXLibRunningApplications(std::map<pid_t, ax_application> *AXApplications)
{
    std::map<pid_t, std::string> List = SharedWorkspaceRunningApplications();
    SharedWorkspaceSetApplicationsPointer(AXApplications);

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

#endif
