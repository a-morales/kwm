#ifndef AXLIB_MAIN_H
#define AXLIB_MAIN_H

#include "observer.h"
#include "element.h"
#include "window.h"
#include "application.h"
#include "sharedworkspace.h"
#include "event.h"

/*
 * NOTE(koekeishiya):
 *        The following functions use the AXLibApplications pointer
 *        to access known ax_application instances.
 *
 *        Before any of these functions can be called, the AXLibApplications
 *        pointer must be set by calling 'AXLibInit(..)'
 *
 *        To populate the ax_application map, the function 'AXLibRunningApplications()'
 *        must be used. This function will query the OSX API and store information about
 *        new applications and their respective windows, in ax_application and ax_window
 *        structs. This function can only retrieve information for the active space!
 * */

ax_application *AXLibGetApplicationByPID(pid_t PID);

ax_application * AXLibGetFocusedApplication();
ax_window *AXLibGetFocusedWindow(ax_application *Application);
void AXLibSetFocusedWindow(ax_window *Window);

void AXLibRunningApplications();
void AXLibInit(std::map<pid_t, ax_application> *AXApplications);

#endif
