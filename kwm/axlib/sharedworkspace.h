#ifndef AXLIB_SHAREDWORKSPACE_H
#define AXLIB_SHAREDWORKSPACE_H

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <map>

#include "application.h"

void SharedWorkspaceSetApplicationsPointer(std::map<pid_t, ax_application> *Apps);
std::vector<pid_t> SharedWorkspaceRunningApplications();
void SharedWorkspaceDidLaunchApplication(pid_t PID);
void SharedWorkspaceDidTerminateApplication(pid_t PID);

#endif
