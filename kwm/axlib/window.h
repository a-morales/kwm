#ifndef AXLIB_WINDOW_H
#define AXLIB_WINDOW_H

#include <Carbon/Carbon.h>
#include "types.h"

struct ax_window_role
{
    CFTypeRef Role;
    CFTypeRef Subrole;
};

struct ax_window
{
    ax_application *Application;
    ax_window_role Type;

    int ID;
    bool Movable;
    bool Resizable;
    AXUIElementRef Ref;

};

ax_window AXLibConstructWindow(ax_application *Application, AXUIElementRef WindowRef);
void AXLibDestroyWindow(ax_window *AXWindow);

#endif
