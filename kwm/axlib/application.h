#ifndef AXLIB_APPLICATION_H
#define AXLIB_APPLICATION_H

#include <Carbon/Carbon.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>
#include <map>

#include "window.h"

struct ax_application
{
    AXUIElementRef Ref;
    int PID;

    ProcessSerialNumber PSN;

    std::vector<ax_window> Windows;
    ax_window *Focus;
};

std::map<pid_t, ax_application> AXLibRunningApplications();
ax_application AXLibConstructApplication(int PID);
void AXLibDestroyApplication(ax_application *Application);

void AXLibAddApplicationWindows(ax_application *Application);
void AXLibRemoveApplicationWindows(ax_application *Application);

#endif
