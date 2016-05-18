#ifndef AXLIB_WINDOW_H
#define AXLIB_WINDOW_H

#include <Carbon/Carbon.h>

struct ax_window_role
{
    CFTypeRef Role;
    CFTypeRef Subrole;
};

struct ax_window
{
    AXUIElementRef Ref;
    int ID;

    bool Movable;
    bool Resizable;

    ax_window_role Type;
};

ax_window AXLibConstructWindow(AXUIElementRef WindowRef);
void AXLibDestroyWindow(ax_window *AXWindow);

#endif
