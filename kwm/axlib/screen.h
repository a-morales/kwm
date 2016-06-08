#ifndef AXLIB_SCREEN_H
#define AXLIB_SCREEN_H

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

struct ax_space
{
    CFStringRef Identifier;
    CGSSpaceID ID;
    CGSSpaceType Type;
};

struct ax_screen
{
    CFStringRef Identifier;
    CGDirectDisplayID ID;
    CGRect Frame;

    ax_space *Space;
    std::map<CGSSpaceID, ax_space> Spaces;
};

void AXLibActiveScreens();
ax_screen *AXLibMainScreen();

#endif
