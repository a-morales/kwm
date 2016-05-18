#ifndef AXLIB_APPLICATION_H
#define AXLIB_APPLICATION_H

#include "window.h"

struct ax_application
{
    AXUIElementRef Ref;
    int PID;

    // std::vector<ax_window> Windows;
    ax_window *Focus;
};

ax_application AXLibConstructApplication();

#endif
