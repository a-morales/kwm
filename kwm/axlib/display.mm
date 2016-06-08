#include "display.h"
#include <Cocoa/Cocoa.h>

#define internal static
#define CGSDefaultConnection _CGSDefaultConnection()

typedef int CGSConnectionID;
extern "C" CGSConnectionID _CGSDefaultConnection(void);
extern "C" CGSSpaceType CGSSpaceGetType(CGSConnectionID CID, CGSSpaceID SID);
extern "C" CFArrayRef CGSCopyManagedDisplaySpaces(const CGSConnectionID CID);
extern "C" CGDirectDisplayID CGSMainDisplayID(void);

internal std::map<CGDirectDisplayID, ax_display> Displays;
internal unsigned int MaxDisplayCount = 5;
internal unsigned int ActiveDisplayCount = 0;

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

/* NOTE(koekeishiya): Find the UUID of the active space for a given ax_display. */
internal CFStringRef
AXLibGetActiveSpaceIdentifier(ax_display *Display)
{
    CFStringRef ActiveSpace = NULL;
    NSString *CurrentIdentifier = (__bridge NSString *)Display->Identifier;

    CFArrayRef DisplayDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *DisplayDictionary in (__bridge NSArray *)DisplayDictionaries)
    {
        NSString *DisplayIdentifier = DisplayDictionary[@"Display Identifier"];
        if([DisplayIdentifier isEqualToString:CurrentIdentifier])
        {
            ActiveSpace = (__bridge CFStringRef) [[NSString alloc] initWithString:DisplayDictionary[@"Current Space"][@"uuid"]];
            break;
        }
    }

    CFRelease(DisplayDictionaries);
    return ActiveSpace;
}

/* NOTE(koekeishiya): Find the CGSSpaceID of the active space for a given ax_display. */
internal CGSSpaceID
AXLibGetActiveSpaceID(ax_display *Display)
{
    CGSSpaceID ActiveSpace = 0;
    NSString *CurrentIdentifier = (__bridge NSString *)Display->Identifier;

    CFArrayRef DisplayDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *DisplayDictionary in (__bridge NSArray *)DisplayDictionaries)
    {
        NSString *DisplayIdentifier = DisplayDictionary[@"Display Identifier"];
        if([DisplayIdentifier isEqualToString:CurrentIdentifier])
        {
            ActiveSpace = [DisplayDictionary[@"Current Space"][@"id64"] intValue];
            break;
        }
    }

    CFRelease(DisplayDictionaries);
    return ActiveSpace;
}

/* NOTE(koekeishiya): Find the active ax_space for a given ax_display. */
internal inline ax_space *
AXLibGetActiveSpace(ax_display *Display)
{
    CGSSpaceID ActiveSpace = AXLibGetActiveSpaceID(Display);
    return &Display->Spaces[ActiveSpace];
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

/* NOTE(koekeishiya): Constructs ax_space structs for every space of a given ax_display. */
internal void
AXLibConstructSpacesForDisplay(ax_display *Display)
{
    NSString *CurrentIdentifier = (__bridge NSString *)Display->Identifier;
    CFArrayRef DisplayDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *DisplayDictionary in (__bridge NSArray *)DisplayDictionaries)
    {
        NSString *DisplayIdentifier = DisplayDictionary[@"Display Identifier"];
        if([DisplayIdentifier isEqualToString:CurrentIdentifier])
        {
            NSDictionary *SpaceDictionaries = DisplayDictionary[@"Spaces"];
            for(NSDictionary *SpaceDictionary in (__bridge NSArray *)SpaceDictionaries)
            {
                CGSSpaceID SpaceID = [SpaceDictionary[@"id64"] intValue];
                CGSSpaceType SpaceType = [SpaceDictionary[@"type"] intValue];
                CFStringRef SpaceUUID = (__bridge CFStringRef) [[NSString alloc] initWithString:SpaceDictionary[@"uuid"]];
                Display->Spaces[SpaceID] = AXLibConstructSpace(SpaceUUID, SpaceID, SpaceType);
            }
            break;
        }
    }

    CFRelease(DisplayDictionaries);
}

/* NOTE(koekeishiya): Initializes a ax_display for a given CGDirectDisplay. Also populates the list of spaces. */
internal ax_display
AXLibConstructDisplay(CGDirectDisplayID DisplayID)
{
    ax_display Display = {};

    Display.ID = DisplayID;
    Display.Identifier = AXLibGetDisplayIdentifier(DisplayID);
    Display.Frame = CGDisplayBounds(DisplayID);
    AXLibConstructSpacesForDisplay(&Display);
    Display.Space = AXLibGetActiveSpace(&Display);

    return Display;
}

/* NOTE(koekeishiya): Populates a map with information about all connected displays. */
void AXLibActiveDisplays()
{
    CGDirectDisplayID *CGDirectDisplayList = (CGDirectDisplayID *) malloc(sizeof(CGDirectDisplayID) * MaxDisplayCount);
    CGGetActiveDisplayList(MaxDisplayCount, CGDirectDisplayList, &ActiveDisplayCount);

    for(std::size_t Index = 0; Index < ActiveDisplayCount; ++Index)
    {
        CGDirectDisplayID ID = CGDirectDisplayList[Index];
        Displays[ID] = AXLibConstructDisplay(ID);
    }

    free(CGDirectDisplayList);

    /* NOTE(koekeishiya): Print the recorded ax_display information. */
    std::map<CGDirectDisplayID, ax_display>::iterator DisplayIt;
    std::map<CGSSpaceID, ax_space>::iterator SpaceIt;
    for(DisplayIt = Displays.begin(); DisplayIt != Displays.end(); ++DisplayIt)
    {
        ax_display *Display = &DisplayIt->second;
        printf("ActiveCGSSpaceID: %d\n", Display->Space->ID);
        for(SpaceIt = Display->Spaces.begin(); SpaceIt != Display->Spaces.end(); ++SpaceIt)
        {
            printf("CGSSpaceID: %d, CGSSpaceType: %d\n", SpaceIt->second.ID, SpaceIt->second.Type);
            CFShow(SpaceIt->second.Identifier);
        }
    }
}

/* NOTE(koekeishiya): The main display is the display which currently holds the window that accepts key-input. */
ax_display *AXLibMainDisplay()
{
    CGDirectDisplayID MainDisplay = CGSMainDisplayID();
    return &Displays[MainDisplay];
}
