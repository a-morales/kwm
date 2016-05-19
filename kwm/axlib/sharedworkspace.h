#ifndef AXLIB_SHAREDWORKSPACE_H
#define AXLIB_SHAREDWORKSPACE_H

#include <sys/types.h>
#include <unistd.h>
#include <vector>

std::vector<pid_t> SharedWorkspaceRunningApplications();

#endif
