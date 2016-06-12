#ifndef AXLIB_DISPLAY_H
#define AXLIB_DISPLAY_H

#include <Carbon/Carbon.h>
#include <string>
#include <map>


/* NOTE(koekeishiya): User controlled spaces */
#define kCGSSpaceUser 0

/* NOTE(koekeishiya): System controlled spaces (dashboard) */
#define kCGSSpaceSystem 2

/* NOTE(koekeishiya): Fullscreen applications */
#define kCGSSpaceFullscreen 4

typedef int CGSSpaceID;
typedef int CGSSpaceType;

struct ax_window;
struct ax_space
{
    CFStringRef Identifier;
    CGSSpaceID ID;
    CGSSpaceType Type;
};

struct ax_display
{
    unsigned int ArrangementID;
    CFStringRef Identifier;
    CGDirectDisplayID ID;
    CGRect Frame;

    ax_space *Space;
    std::map<CGSSpaceID, ax_space> Spaces;
};

void AXLibInitializeDisplays(std::map<CGDirectDisplayID, ax_display> *AXDisplays);
ax_space * AXLibGetActiveSpace(ax_display *Display);

ax_display *AXLibMainDisplay();
ax_display *AXLibWindowDisplay(ax_window *Window);
ax_display *AXLibNextDisplay(ax_display *Display);
ax_display *AXLibPreviousDisplay(ax_display *Display);

unsigned int AXLibDesktopIDFromCGSSpaceID(ax_display *Display, CGSSpaceID SpaceID);
CGSSpaceID AXLibCGSSpaceIDFromDesktopID(ax_display *Display, unsigned int DesktopID);

#endif
