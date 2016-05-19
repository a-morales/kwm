#ifndef AXLIB_MAIN_H
#define AXLIB_MAIN_H

#include "types.h"
#include "observer.h"
#include "element.h"
#include "window.h"
#include "application.h"
#include "sharedworkspace.h"

#define internal static

internal inline bool
AXLibIsApplicationCached(std::map<pid_t, ax_application> *AXApplications, pid_t PID)
{
    std::map<pid_t, ax_application>::iterator It = AXApplications->find(PID);
    return It != AXApplications->end();
}

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
        if(!AXLibIsApplicationCached(AXApplications, PID))
        {
            (*AXApplications)[PID] = AXLibConstructApplication(PID, Name);
            AXLibAddApplicationObserver(&(*AXApplications)[PID]);
        }

        AXLibAddApplicationWindows(&(*AXApplications)[PID]);
    }
}

#endif
