#include "cursor.h"
#include "axlib/axlib.h"
#include "space.h"

#define internal static
#define local_persist static

extern ax_application *FocusedApplication;
extern kwm_toggles KWMToggles;

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
    if(KWMToggles.UseMouseFollowsFocus)
    {
        CGWarpMouseCursorPosition(CGPointMake(Window->Position.x + Window->Size.width / 2,
                                              Window->Position.y + Window->Size.height / 2));
    }
}

void FocusWindowBelowCursor()
{
    std::vector<ax_window *> Windows = AXLibGetAllVisibleWindowsOrdered();
    for(std::size_t Index = 0; Index < Windows.size(); ++Index)
    {
        ax_window *Window = Windows[Index];
        if(IsWindowBelowCursor(Window))
        {
           if(FocusedApplication == Window->Application)
           {
               if(FocusedApplication->Focus != Window)
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
