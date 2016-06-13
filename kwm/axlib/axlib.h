#ifndef AXLIB_MAIN_H
#define AXLIB_MAIN_H

#include "observer.h"
#include "element.h"
#include "window.h"
#include "application.h"
#include "display.h"
#include "sharedworkspace.h"
#include "event.h"
#include "carbon.h"

/*
 * NOTE(koekeishiya):
 *        AXlib requires the use of the 'ax_state' struct and a pointer to a variable of this
 *        type should be passed when calling AXLibInit(..).
 *
 *        AXLibInit(..) will perform some setup that has to happen before any of the following
 *        functions can be called. Trying to call any of the below functions before AXLibInit(..)
 *        will result in undefined behaviour, most likely a segmentation fault.
 *
 *        To populate the ax_application map, the function 'AXLibRunningApplications()'
 *        can be used. This function will query the OSX API and store information about
 *        applications and their respective windows, in ax_application and ax_window
 *        structs. This function can only retrieve information for the active space!
 *
 *        Subsequent calls to the 'AXLibRunningApplications()' function will initialize
 *        any applications that have yet to be detected, as well as add any new windows
 *        that an application has spawned, if they for some reason failed to notify us
 *        through the 'kAXWindowCreatedNotification'.
 *
 *        To be able to correctly monitor windows for multiple spaces, we assume that
 *        a call to 'AXLibRunningApplications()' followed by 'AXLibGetVisibleWindows()'
 *        should happen every time a space transition occurs on the active monitor.
 * */

struct ax_state
{
    carbon_event_handler Carbon;
    std::map<pid_t, ax_application> Applications;
    std::map<CGDirectDisplayID, ax_display> Displays;
};

ax_application *AXLibGetApplicationByPID(pid_t PID);

ax_application * AXLibGetFocusedApplication();
ax_window *AXLibGetFocusedWindow(ax_application *Application);
void AXLibSetFocusedWindow(ax_window *Window);

std::vector<ax_window *> AXLibGetAllKnownWindows();
std::vector<ax_window *> AXLibGetAllVisibleWindows();
std::vector<ax_window *> AXLibGetAllVisibleWindowsOrdered();
void AXLibRunningApplications();
void AXLibInit(ax_state *State);

#endif
