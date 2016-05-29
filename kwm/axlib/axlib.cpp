#include "axlib.h"

#define internal static
#define local_persist static

internal std::map<pid_t, ax_application> *AXApplications;

internal bool
AXLibIsApplicationCached(pid_t PID)
{
    std::map<pid_t, ax_application>::iterator It = AXApplications->find(PID);
    return It != AXApplications->end();
}

ax_application *AXLibGetFocusedApplication()
{
    local_persist AXUIElementRef SystemWideElement = AXUIElementCreateSystemWide();
    AXUIElementRef Ref = (AXUIElementRef) AXLibGetWindowProperty(SystemWideElement, kAXFocusedApplicationAttribute);

    if(Ref)
    {
        pid_t PID;
        AXUIElementGetPid(Ref, &PID);
        CFRelease(Ref);

        if(AXLibIsApplicationCached(PID))
            return &(*AXApplications)[PID];
    }

    return NULL;
}

ax_window *AXLibGetFocusedWindow(ax_application *Application)
{
    AXUIElementRef Ref = (AXUIElementRef) AXLibGetWindowProperty(Application->Ref, kAXFocusedWindowAttribute);
    if(Ref)
    {
        uint32_t WID = AXLibGetWindowID(Ref);
        CFRelease(Ref);

        ax_window *Window = AXLibFindApplicationWindow(Application, WID);
        return Window;
    }

    return NULL;
}

void AXLibSetFocusedWindow(ax_window *Window)
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

void AXLibRunningApplications()
{
    std::map<pid_t, std::string> List = SharedWorkspaceRunningApplications();

    std::map<pid_t, std::string>::iterator It;
    for(It = List.begin(); It != List.end(); ++It)
    {
        pid_t PID = It->first;
        if(!AXLibIsApplicationCached(PID))
        {
            std::string Name = It->second;
            (*AXApplications)[PID] = AXLibConstructApplication(PID, Name);
            AXLibAddApplicationObserver(&(*AXApplications)[PID]);
        }

        AXLibAddApplicationWindows(&(*AXApplications)[PID]);
    }
}

void AXLibInit(std::map<pid_t, ax_application> *Apps)
{
    AXApplications = Apps;
    SharedWorkspaceSetApplicationsPointer(AXApplications);
}
