#include "screen.h"
#include <Cocoa/Cocoa.h>

#define internal static
#define CGSDefaultConnection _CGSDefaultConnection()

typedef int CGSConnectionID;
extern "C" CGSConnectionID _CGSDefaultConnection(void);
extern "C" CGSSpaceType CGSSpaceGetType(CGSConnectionID CID, CGSSpaceID SID);
extern "C" CFArrayRef CGSCopyManagedDisplaySpaces(const CGSConnectionID CID);
extern "C" CGDirectDisplayID CGSMainDisplayID(void);

internal std::map<CGDirectDisplayID, ax_screen> Screens;
internal unsigned int MaxScreenCount = 5;
internal unsigned int ActiveScreenCount = 0;

/* NOTE(koekeishiya): Find the UUID for a given CGDirectDisplay. */
internal CFStringRef
AXLibGetDisplayIdentifier(int DisplayID)
{
    CFUUIDRef DisplayUUID = CGDisplayCreateUUIDFromDisplayID(DisplayID);
    if(DisplayUUID)
    {
        CFStringRef Identifier = CFUUIDCreateString(NULL, DisplayUUID);
        CFRelease(DisplayUUID);
        return Identifier;
    }

    return NULL;
}

/* NOTE(koekeishiya): Find the UUID of the active space for a given ax_screen. */
internal CFStringRef
AXLibGetActiveSpaceIdentifier(ax_screen *Screen)
{
    CFStringRef ActiveSpace = NULL;
    NSString *CurrentIdentifier = (__bridge NSString *)Screen->Identifier;

    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            ActiveSpace = (__bridge CFStringRef) [[NSString alloc] initWithString:ScreenDictionary[@"Current Space"][@"uuid"]];
            break;
        }
    }

    CFRelease(ScreenDictionaries);
    return ActiveSpace;
}

/* NOTE(koekeishiya): Find the CGSSpaceID of the active space for a given ax_screen. */
internal CGSSpaceID
AXLibGetActiveSpaceID(ax_screen *Screen)
{
    CGSSpaceID ActiveSpace = 0;
    NSString *CurrentIdentifier = (__bridge NSString *)Screen->Identifier;

    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            ActiveSpace = [ScreenDictionary[@"Current Space"][@"id64"] intValue];
            break;
        }
    }

    CFRelease(ScreenDictionaries);
    return ActiveSpace;
}

/* NOTE(koekeishiya): Find the active ax_space for a given ax_screen. */
internal inline ax_space *
AXLibGetActiveSpace(ax_screen *Screen)
{
    CGSSpaceID ActiveSpace = AXLibGetActiveSpaceID(Screen);
    return &Screen->Spaces[ActiveSpace];
}

internal ax_space
AXLibConstructSpace(CFStringRef Identifier, CGSSpaceID SpaceID, CGSSpaceType SpaceType)
{
    ax_space Space;

    Space.Identifier = Identifier;
    Space.ID = SpaceID;
    Space.Type = SpaceType;

    return Space;
}

/* NOTE(koekeishiya): Constructs ax_space structs for every space of a given ax_screen. */
internal void
AXLibConstructSpacesForScreen(ax_screen *Screen)
{
    NSString *CurrentIdentifier = (__bridge NSString *)Screen->Identifier;
    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            NSDictionary *SpaceDictionaries = ScreenDictionary[@"Spaces"];
            for(NSDictionary *SpaceDictionary in (__bridge NSArray *)SpaceDictionaries)
            {
                CGSSpaceID SpaceID = [SpaceDictionary[@"id64"] intValue];
                CGSSpaceType SpaceType = [SpaceDictionary[@"type"] intValue];
                CFStringRef SpaceUUID = (__bridge CFStringRef) [[NSString alloc] initWithString:SpaceDictionary[@"uuid"]];
                Screen->Spaces[SpaceID] = AXLibConstructSpace(SpaceUUID, SpaceID, SpaceType);
            }
            break;
        }
    }

    CFRelease(ScreenDictionaries);
}

/* NOTE(koekeishiya): Initializes a ax_screen for a given CGDirectDisplay. Also populates the list of spaces. */
internal ax_screen
AXLibConstructScreen(CGDirectDisplayID DisplayID)
{
    ax_screen Screen = {};

    Screen.ID = DisplayID;
    Screen.Identifier = AXLibGetDisplayIdentifier(DisplayID);
    Screen.Frame = CGDisplayBounds(DisplayID);
    AXLibConstructSpacesForScreen(&Screen);
    Screen.Space = AXLibGetActiveSpace(&Screen);

    return Screen;
}

/* NOTE(koekeishiya): Populates a map with information about all connected screens. */
void AXLibActiveScreens()
{
    CGDirectDisplayID *CGDirectDisplayList = (CGDirectDisplayID *) malloc(sizeof(CGDirectDisplayID) * MaxScreenCount);
    CGGetActiveDisplayList(MaxScreenCount, CGDirectDisplayList, &ActiveScreenCount);

    for(std::size_t Index = 0; Index < ActiveScreenCount; ++Index)
    {
        CGDirectDisplayID DisplayID = CGDirectDisplayList[Index];
        Screens[DisplayID] = AXLibConstructScreen(DisplayID);
    }

    free(CGDirectDisplayList);

    /* NOTE(koekeishiya): Print the recorded ax_screen information. */
    /*
    std::map<CGDirectDisplayID, ax_screen>::iterator ScreenIt;
    std::map<CGSSpaceID, ax_space>::iterator SpaceIt;
    for(ScreenIt = Screens.begin(); ScreenIt != Screens.end(); ++ScreenIt)
    {
        ax_screen *Screen = &ScreenIt->second;
        printf("ActiveCGSSpaceID: %d\n", Screen->Space->ID);
        for(SpaceIt = Screen->Spaces.begin(); SpaceIt != Screen->Spaces.end(); ++SpaceIt)
        {
            printf("CGSSpaceID: %d, CGSSpaceType: %d\n", SpaceIt->second.ID, SpaceIt->second.Type);
            CFShow(SpaceIt->second.Identifier);
        }
    }
    */
}

/* NOTE(koekeishiya): The main screen is the screen which currently holds the window that accepts key-input. */
ax_screen *AXLibMainScreen()
{
    CGDirectDisplayID MainDisplay = CGSMainDisplayID();
    return &Screens[MainDisplay];
}
