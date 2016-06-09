#include "window.h"
#include "element.h"
#include "display.h"

#define internal static
#define local_persist static

ax_window AXLibConstructWindow(ax_application *Application, AXUIElementRef WindowRef)
{
    ax_window Window = {};

    Window.Ref = WindowRef;
    CFRetain(WindowRef);

    Window.Application = Application;
    Window.ID = AXLibGetWindowID(WindowRef);
    Window.Name = AXLibGetWindowTitle(WindowRef);

    Window.Position = AXLibGetWindowPosition(WindowRef);
    Window.Size = AXLibGetWindowSize(WindowRef);

    if(AXLibIsWindowMovable(WindowRef))
        AXLibAddFlags(&Window, AXWindow_Movable);

    if(AXLibIsWindowResizable(WindowRef))
        AXLibAddFlags(&Window, AXWindow_Resizable);

    AXLibGetWindowRole(WindowRef, &Window.Type.Role);
    AXLibGetWindowSubrole(WindowRef, &Window.Type.Subrole);

    return Window;
}

/* TODO(koekeishiya): Need to implement support for adding other roles that should be accepted
                      by this particular ax_window instance. */
bool AXLibIsWindowStandard(ax_window *Window)
{
    bool Result = ((CFEqual(Window->Type.Role, kAXWindowRole)) &&
                   (CFEqual(Window->Type.Subrole, kAXStandardWindowSubrole)));
    return Result;
}

ax_display *AXLibGetWindowDisplay(ax_window *Window, std::map<CGDirectDisplayID, ax_display> *Displays)
{
    CGRect Frame = { Window->Position, Window->Size };
    CGFloat HighestVolume = 0;
    ax_display *BestDisplay = NULL;

    std::map<CGDirectDisplayID, ax_display>::iterator It;
    for(It = Displays->begin(); It != Displays->end(); ++It)
    {
        ax_display *Display = &It->second;
        CGRect Intersection = CGRectIntersection(Frame, Display->Frame);
        CGFloat Volume = Intersection.size.width * Intersection.size.height;

        if(Volume > HighestVolume)
        {
            HighestVolume = Volume;
            BestDisplay = Display;
        }
    }

    return BestDisplay;
}

void AXLibDestroyWindow(ax_window *Window)
{
    if(Window->Ref)
        CFRelease(Window->Ref);

    if(Window->Type.Role)
        CFRelease(Window->Type.Role);

    if(Window->Type.Subrole)
        CFRelease(Window->Type.Subrole);

    Window->Ref = NULL;
    Window->Type.Role = NULL;
    Window->Type.Subrole = NULL;
    Window->Application = NULL;
}
