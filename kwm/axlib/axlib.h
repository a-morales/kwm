#ifndef AXLIB_MAIN_H
#define AXLIB_MAIN_H

#include "types.h"
#include "observer.h"
#include "element.h"
#include "window.h"
#include "application.h"
#include "sharedworkspace.h"

#define internal static
#define local_persist static

internal inline bool
AXLibIsApplicationCached(std::map<pid_t, ax_application> *AXApplications, pid_t PID)
{
    std::map<pid_t, ax_application>::iterator It = AXApplications->find(PID);
    return It != AXApplications->end();
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

        if(AXLibIsApplicationCached(AXApplications, PID))
            return &(*AXApplications)[PID];
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
AXLibRunningApplications(std::map<pid_t, ax_application> *AXApplications)
{
    std::map<pid_t, std::string> List = SharedWorkspaceRunningApplications();
    SharedWorkspaceSetApplicationsPointer(AXApplications);

    std::map<pid_t, std::string>::iterator It;
    for(It = List.begin(); It != List.end(); ++It)
    {
        pid_t PID = It->first;
        std::string Name = It->second;
        if(!AXLibIsApplicationCached(AXApplications, PID))
        {
            (*AXApplications)[PID] = AXLibConstructApplication(PID, Name);
            AXLibAddApplicationObserver(&(*AXApplications)[PID]);
        }

        AXLibAddApplicationWindows(&(*AXApplications)[PID]);
    }
}

#endif
