#ifndef AXLIB_MAIN_H
#define AXLIB_MAIN_H

#include "types.h"
#include "observer.h"
#include "element.h"
#include "window.h"
#include "application.h"
#include "sharedworkspace.h"

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

#define internal static
#define local_persist static

internal std::map<pid_t, ax_application> *AXLibApplications;

internal inline bool
AXLibIsApplicationCached(pid_t PID)
{
    std::map<pid_t, ax_application>::iterator It = AXLibApplications->find(PID);
    return It != AXLibApplications->end();
}

inline ax_application *
AXLibGetFocusedApplication()
{
    local_persist AXUIElementRef SystemWideElement = AXUIElementCreateSystemWide();
    AXUIElementRef Ref = (AXUIElementRef) AXLibGetWindowProperty(SystemWideElement, kAXFocusedApplicationAttribute);

    if(Ref)
    {
        pid_t PID;
        AXUIElementGetPid(Ref, &PID);
        CFRelease(Ref);

        if(AXLibIsApplicationCached(PID))
            return &(*AXLibApplications)[PID];
    }

    return NULL;
}

inline ax_window *
AXLibGetFocusedWindow(ax_application *Application)
{
    AXUIElementRef Ref = (AXUIElementRef) AXLibGetWindowProperty(Application->Ref, kAXFocusedWindowAttribute);
    if(Ref)
    {
        int WID = AXLibGetWindowID(Ref);
        CFRelease(Ref);

        ax_window *Window = AXLibFindApplicationWindow(Application, WID);
        return Window;
    }

    return NULL;
}

inline void
AXLibSetFocusedWindow(ax_window *Window)
{
    AXLibSetWindowProperty(Window->Ref, kAXMainAttribute, kCFBooleanTrue);
    AXLibSetWindowProperty(Window->Ref, kAXFocusedAttribute, kCFBooleanTrue);
    AXUIElementPerformAction(Window->Ref, kAXRaiseAction);

    if(!AXLibIsApplicationActive(Window->Application))
        AXLibActivateApplication(Window->Application);

    /* TODO(koekeishiya): Confirm that the following behaviour is performed by
                          the 'AXLibActivateApplication' function call.

        if(KWMMode.Focus != FocusModeAutofocus && KWMMode.Focus != FocusModeStandby)
        SetFrontProcessWithOptions(&Window->Application->PSN, kSetFrontProcessFrontWindowOnly);
    */
}

inline void
AXLibRunningApplications()
{
    std::map<pid_t, std::string> List = SharedWorkspaceRunningApplications();

    std::map<pid_t, std::string>::iterator It;
    for(It = List.begin(); It != List.end(); ++It)
    {
        pid_t PID = It->first;
        std::string Name = It->second;
        if(!AXLibIsApplicationCached(PID))
        {
            (*AXLibApplications)[PID] = AXLibConstructApplication(PID, Name);
            AXLibAddApplicationObserver(&(*AXLibApplications)[PID]);
        }

        AXLibAddApplicationWindows(&(*AXLibApplications)[PID]);
    }
}

inline void
AXLibInit(std::map<pid_t, ax_application> *AXApplications)
{
    AXLibApplications = AXApplications;
    SharedWorkspaceSetApplicationsPointer(AXApplications);
}

#endif
