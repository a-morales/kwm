#include "cursor.h"
#include "axlib/axlib.h"
#include "space.h"

#define internal static
#define local_persist static

extern ax_application *FocusedApplication;

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

void FocusWindowBelowCursor()
{
    ax_window *FocusedWindow = FocusedApplication->Focus;
    if(FocusedWindow && IsWindowBelowCursor(FocusedWindow))
        return;

    std::vector<ax_window *> Windows = AXLibGetAllVisibleWindowsOrdered();
    for(std::size_t Index = 0; Index < Windows.size(); ++Index)
    {
        ax_window *Window = Windows[Index];

        /* Note(koekeishiya): Allow focus-follows-mouse to ignore Launchpad */
        if(Window->Application->Name == "Dock" &&
           Window->Name  == "LPSpringboard")
            return;

        /* Note(koekeishiya): Allow focus-follows-mouse to work when the dock is visible */
        if(Window->Application->Name == "Dock" &&
           Window->Position.x == 0 &&
           Window->Position.y == 0)
            continue;

        if(IsWindowBelowCursor(Window))
        {
           if(FocusedWindow != Window)
                AXLibSetFocusedWindow(Window);
            return;
        }
    }
}
