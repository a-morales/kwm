#import "Cocoa/Cocoa.h"
#import "types.h"
#include "axlib/axlib.h"
#include "border.h"

extern int GetSpaceFromName(ax_display *Display, std::string Name);

extern kwm_focus KWMFocus;
extern kwm_screen KWMScreen;

/* NOTE(koekeishiya): Stubs to prevent compile errors. */
bool IsWindowOnSpace(int WindowID, int CGSpaceID) { }

int GetActiveSpaceOfDisplay(screen_info *Screen) { }

int GetSpaceNumberFromCGSpaceID(screen_info *Screen, int CGSpaceID) { }

int GetCGSpaceIDFromSpaceNumber(screen_info *Screen, int SpaceID) { }
/* --------------------------------------------------- */

int GetNumberOfSpacesOfDisplay(screen_info *Screen)
{
    int Result = 0;
    NSString *CurrentIdentifier = (__bridge NSString *)Screen->Identifier;

    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if ([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            NSArray *Spaces = ScreenDictionary[@"Spaces"];
            Result = CFArrayGetCount((__bridge CFArrayRef)Spaces);
        }
    }

    CFRelease(ScreenDictionaries);
    return Result;
}

extern "C" int CGSRemoveWindowsFromSpaces(int cid, CFArrayRef windows, CFArrayRef spaces);
extern "C" int CGSAddWindowsToSpaces(int cid, CFArrayRef windows, CFArrayRef spaces);

void ActivateSpaceWithoutTransition(std::string SpaceID)
{
    ax_display *Display = AXLibMainDisplay();
    if(Display)
    {
        DEBUG("SPACEID STR: " << SpaceID);
        int TotalSpaces = AXLibDisplaySpacesCount(Display);
        int ActiveSpace = AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID);
        int DestinationSpaceID = ActiveSpace;
        if(SpaceID == "left")
        {
            DestinationSpaceID = ActiveSpace > 1 ? ActiveSpace-1 : 1;
        }
        else if(SpaceID == "right")
        {
            DestinationSpaceID = ActiveSpace < TotalSpaces ? ActiveSpace+1 : TotalSpaces;
        }
        else
        {
            int LookupSpace = GetSpaceFromName(Display, SpaceID);
            if(LookupSpace != -1)
                DestinationSpaceID = AXLibDesktopIDFromCGSSpaceID(Display, LookupSpace);
            else
                DestinationSpaceID = std::atoi(SpaceID.c_str());
        }

        DEBUG("DESTINATION SPACE: " << DestinationSpaceID);
        if(DestinationSpaceID != ActiveSpace &&
           DestinationSpaceID > 0 && DestinationSpaceID <= TotalSpaces)
        {
            int CGSSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, DestinationSpaceID);
            AXLibInstantSpaceTransition(Display, CGSSpaceID);
        }
    }
}

void RemoveWindowFromSpace(int SpaceID, int WindowID)
{
    NSArray *NSArrayWindow = @[ @(WindowID) ];
    NSArray *NSArraySourceSpace = @[ @(SpaceID) ];
    CGSRemoveWindowsFromSpaces(CGSDefaultConnection, (__bridge CFArrayRef)NSArrayWindow, (__bridge CFArrayRef)NSArraySourceSpace);
}

void AddWindowToSpace(int SpaceID, int WindowID)
{
    NSArray *NSArrayWindow = @[ @(WindowID) ];
    NSArray *NSArrayDestinationSpace = @[ @(SpaceID) ];
    CGSAddWindowsToSpaces(CGSDefaultConnection, (__bridge CFArrayRef)NSArrayWindow, (__bridge CFArrayRef)NSArrayDestinationSpace);
}

void MoveWindowBetweenSpaces(int SourceSpaceID, int DestinationSpaceID, int WindowID)
{
    int SourceCGSpaceID = GetCGSpaceIDFromSpaceNumber(KWMScreen.Current, SourceSpaceID);
    int DestinationCGSpaceID = GetCGSpaceIDFromSpaceNumber(KWMScreen.Current, DestinationSpaceID);
    RemoveWindowFromSpace(SourceCGSpaceID, WindowID);
    AddWindowToSpace(DestinationCGSpaceID, WindowID);
}

void MoveFocusedWindowToSpace(std::string SpaceID)
{
    if(KWMScreen.Current && KWMFocus.Window)
    {
        int TotalSpaces = GetNumberOfSpacesOfDisplay(KWMScreen.Current);
        int ActiveSpace = GetSpaceNumberFromCGSpaceID(KWMScreen.Current, KWMScreen.Current->ActiveSpace);
        int DestinationSpaceID = ActiveSpace;
        if(SpaceID == "left")
        {
            DestinationSpaceID = ActiveSpace > 1 ? ActiveSpace-1 : 1;
        }
        else if(SpaceID == "right")
        {
            DestinationSpaceID = ActiveSpace < TotalSpaces ? ActiveSpace+1 : TotalSpaces;
        }
        else
        {
            int LookupSpace = -1; //GetSpaceFromName(KWMScreen.Current, SpaceID);
            if(LookupSpace != -1)
                DestinationSpaceID = GetSpaceNumberFromCGSpaceID(KWMScreen.Current, LookupSpace);
            else
                DestinationSpaceID = std::atoi(SpaceID.c_str());
        }

        MoveWindowBetweenSpaces(ActiveSpace, DestinationSpaceID, KWMFocus.Window->WID);
    }
}
