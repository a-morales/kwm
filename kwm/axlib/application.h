#ifndef AXLIB_APPLICATION_H
#define AXLIB_APPLICATION_H

#include <Carbon/Carbon.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <map>

#include "types.h"
#include "window.h"
#include "observer.h"

struct ax_application
{
    AXUIElementRef Ref;
    std::string Name;
    pid_t PID;

    ProcessSerialNumber PSN;
    ax_observer Observer;

    bool Float;
    ax_window *Focus;
    std::map<uint32_t, ax_window> Windows;
    std::map<uint32_t, ax_window*> VisibleWindows;
};

ax_application AXLibConstructApplication(pid_t PID, std::string Name);
void AXLibDestroyApplication(ax_application *Application);

void AXLibAddApplicationWindows(ax_application *Application);
void AXLibRemoveApplicationWindows(ax_application *Application);

ax_window *AXLibFindApplicationWindow(ax_application *Application, uint32_t WID);
void AXLibAddApplicationWindow(ax_application *Application, ax_window Window);
void AXLibRemoveApplicationWindow(ax_application *Application, uint32_t WID);

void AXLibAddApplicationObserver(ax_application *Application);
void AXLibRemoveApplicationObserver(ax_application *Application);

void AXLibActivateApplication(ax_application *Application);
bool AXLibIsApplicationActive(ax_application *Application);

#endif
