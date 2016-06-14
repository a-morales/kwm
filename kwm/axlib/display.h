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

enum CGSSpaceSelector
{
    kCGSSpaceCurrent = 5,
    kCGSSpaceOther = 6,
    kCGSSpaceAll = 7
};

typedef int CGSSpaceID;
typedef int CGSSpaceType;

struct ax_window;
enum ax_space_flags
{
    AXSpace_DeminimizedTransition = (1 << 0),
};

struct ax_space
{
    CFStringRef Identifier;
    CGSSpaceID ID;
    CGSSpaceType Type;
    uint32_t Flags;
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

inline bool
AXLibHasFlags(ax_space *Space, uint32_t Flag)
{
    bool Result = Space->Flags & Flag;
    return Result;
}

inline void
AXLibAddFlags(ax_space *Space, uint32_t Flag)
{
    Space->Flags |= Flag;
}

inline void
AXLibClearFlags(ax_space *Space, uint32_t Flag)
{
    Space->Flags &= ~Flag;
}


void AXLibInitializeDisplays(std::map<CGDirectDisplayID, ax_display> *AXDisplays);
ax_space * AXLibGetActiveSpace(ax_display *Display);
bool AXLibIsSpaceTransitionInProgress();

ax_display *AXLibMainDisplay();
ax_display *AXLibWindowDisplay(ax_window *Window);
ax_display *AXLibNextDisplay(ax_display *Display);
ax_display *AXLibPreviousDisplay(ax_display *Display);

unsigned int AXLibDesktopIDFromCGSSpaceID(ax_display *Display, CGSSpaceID SpaceID);
CGSSpaceID AXLibCGSSpaceIDFromDesktopID(ax_display *Display, unsigned int DesktopID);
bool AXLibIsWindowOnSpace(ax_window *Window, CGSSpaceID SpaceID);

#endif
