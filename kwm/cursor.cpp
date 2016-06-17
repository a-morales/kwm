#include "cursor.h"
#include "axlib/axlib.h"
#include "space.h"

#define internal static
#define local_persist static

extern ax_application *FocusedApplication;
extern kwm_settings KWMSettings;

internal CGPoint
GetCursorPos()
{
    CGEventRef Event = CGEventCreate(NULL);
    CGPoint Cursor = CGEventGetLocation(Event);
    CFRelease(Event);

    return Cursor;
}

internal bool
IsWindowBelowCursor(ax_window *Window)
{
    CGPoint Cursor = GetCursorPos();
    if(Cursor.x >= Window->Position.x &&
       Cursor.x <= Window->Position.x + Window->Size.width &&
       Cursor.y >= Window->Position.y &&
       Cursor.y <= Window->Position.y + Window->Size.height)
        return true;

    return false;
}

void MoveCursorToCenterOfWindow(ax_window *Window)
{
    if(KWMSettings.UseMouseFollowsFocus)
    {
        CGWarpMouseCursorPosition(CGPointMake(Window->Position.x + Window->Size.width / 2,
                                              Window->Position.y + Window->Size.height / 2));
    }
}

void MoveCursorToCenterOfFocusedWindow()
{
    if(KWMSettings.UseMouseFollowsFocus &&
       FocusedApplication &&
       FocusedApplication->Focus)
        MoveCursorToCenterOfWindow(FocusedApplication->Focus);
}


void FocusWindowBelowCursor()
{
    ax_application *Application = AXLibGetFocusedApplication();
    if(!Application)
        return;

    ax_window *FocusedWindow = Application->Focus;
    if(!FocusedWindow)
        return;

    if(IsWindowBelowCursor(FocusedWindow))
        return;

    std::vector<ax_window *> Windows = AXLibGetAllVisibleWindowsOrdered();
    for(std::size_t Index = 0; Index < Windows.size(); ++Index)
    {
        ax_window *Window = Windows[Index];
        if(IsWindowBelowCursor(Window))
        {
           if(Application == Window->Application)
           {
               if(FocusedWindow != Window)
                   AXLibSetFocusedWindow(Window);
           }
           else
           {
                AXLibSetFocusedWindow(Window);
           }
            return;
        }
    }
}

EVENT_CALLBACK(Callback_AXEvent_MouseMoved)
{
    if(!AXLibIsSpaceTransitionInProgress())
        FocusWindowBelowCursor();
}

