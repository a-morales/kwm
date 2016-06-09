#include "window.h"
#include "element.h"
#include "display.h"

#define internal static
#define local_persist static

ax_window *AXLibConstructWindow(ax_application *Application, AXUIElementRef WindowRef)
{
    ax_window *Window = (ax_window *) malloc(sizeof(ax_window));
    memset(Window, '\0', sizeof(ax_window));

    Window->Ref = (AXUIElementRef) CFRetain(WindowRef);
    Window->Application = Application;
    Window->ID = AXLibGetWindowID(WindowRef);
    Window->Name = AXLibGetWindowTitle(WindowRef);

    Window->Position = AXLibGetWindowPosition(WindowRef);
    Window->Size = AXLibGetWindowSize(WindowRef);

    if(AXLibIsWindowMovable(Window->Ref))
        AXLibAddFlags(Window, AXWindow_Movable);

    if(AXLibIsWindowResizable(Window->Ref))
        AXLibAddFlags(Window, AXWindow_Resizable);

    AXLibGetWindowRole(Window->Ref, &Window->Type.Role);
    AXLibGetWindowSubrole(Window->Ref, &Window->Type.Subrole);

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
    free(Window);
}
