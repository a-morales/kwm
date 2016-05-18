#ifndef AXLIB_APPLICATION_H
#define AXLIB_APPLICATION_H

#include "window.h"
#include <vector>

struct ax_application
{
    AXUIElementRef Ref;
    int PID;

    ProcessSerialNumber PSN;

    std::vector<ax_window> Windows;
    ax_window *Focus;
};

ax_application AXLibConstructApplication();
void AXLibDestroyApplication(ax_application *Application);

void AXLibAddApplicationWindows(ax_application *Application);
void AXLibRemoveApplicationWindows(ax_application *Application)

#endif
