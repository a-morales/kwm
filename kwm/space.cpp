#include "space.h"
#include "display.h"
#include "window.h"
#include "tree.h"
#include "border.h"
#include "keys.h"
#include "helpers.h"

#include "axlib/axlib.h"
extern std::map<CFStringRef, space_info> WindowTree;
extern ax_application *FocusedApplication;

extern kwm_mach KWMMach;
extern kwm_tiling KWMTiling;
extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_toggles KWMToggles;
extern kwm_mode KWMMode;
extern kwm_thread KWMThread;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

extern void AddWindowToSpace(int CGSpaceID, int WindowID);
extern void RemoveWindowFromSpace(int CGSpaceID, int WindowID);

/* NOTE(koekeishiya): Causes segfault if this function is called exactly as the window is destroyed.
 *                    We probably have to use a lock on FocusedApplication->Focus */
void GetTagForMonocleSpace(space_info *Space, std::string &Tag)
{
    tree_node *Node = Space->RootNode;
    bool FoundFocusedWindow = false;
    int FocusedIndex = 0;
    int NumberOfWindows = 0;

    if(Node && FocusedApplication && FocusedApplication->Focus)
    {
        link_node *Link = Node->List;
        while(Link)
        {
            if(!FoundFocusedWindow)
                ++FocusedIndex;

            if(Link->WindowID == FocusedApplication->Focus->ID)
                FoundFocusedWindow = true;

            ++NumberOfWindows;
            Link = Link->Next;
        }
    }

    if(FoundFocusedWindow)
        Tag = "[" + std::to_string(FocusedIndex) + "/" + std::to_string(NumberOfWindows) + "]";
    else
        Tag = "[" + std::to_string(NumberOfWindows) + "]";
}

void GetTagForCurrentSpace(std::string &Tag)
{
    ax_display *Display = AXLibMainDisplay();
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Initialized)
    {
        if(SpaceInfo->Settings.Mode == SpaceModeBSP)
            Tag = "[bsp]";
        else if(SpaceInfo->Settings.Mode == SpaceModeFloating)
            Tag = "[float]";
        else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
            GetTagForMonocleSpace(SpaceInfo, Tag);
    }
    else
    {
        if(KWMMode.Space == SpaceModeBSP)
            Tag = "[bsp]";
        else if(KWMMode.Space == SpaceModeFloating)
            Tag = "[float]";
        else if(KWMMode.Space == SpaceModeMonocle)
            Tag = "[monocle]";
    }
}

void GoToPreviousSpace(bool MoveFocusedWindow)
{
    /*
    if(IsSpaceTransitionInProgress())
        return;

    Assert(KWMScreen.Current);
    if(!KWMScreen.Current->History.empty())
    {
        int CGSpaceID = KWMScreen.Current->History.top();
        KWMScreen.Current->History.pop();

        int Workspace = GetSpaceNumberFromCGSpaceID(KWMScreen.Current, CGSpaceID);
        std::string WorkspaceStr = std::to_string(Workspace);
        KWMScreen.Current->TrackSpaceChange = false;

        if(MoveFocusedWindow)
            MoveFocusedWindowToSpace(WorkspaceStr);
        else
            KwmEmitKeystroke(KWMHotkeys.SpacesKey, WorkspaceStr);
    }
    */
}

bool IsActiveSpaceFloating() { }

bool IsSpaceFloating(int SpaceID) { }

space_info *GetActiveSpaceOfScreen(screen_info *Screen) { }

bool IsSpaceInitializedForScreen(screen_info *Screen) { }

bool DoesSpaceExistInMapOfScreen(screen_info *Screen) { }

bool IsSpaceTransitionInProgress() { }

bool IsActiveSpaceManaged() { }

void ShouldActiveSpaceBeManaged() { }

void FloatFocusedSpace() { }

void UpdateActiveSpace() { }

void SetNameOfActiveSpace(screen_info *Screen, std::string Name) {}

std::string GetNameOfSpace(screen_info *Screen, int CGSpaceID) { }

void TileFocusedSpace(space_tiling_option Mode) { }

space_settings *GetSpaceSettingsForDesktopID(int ScreenID, int DesktopID)
{
    space_identifier Lookup = { ScreenID, DesktopID };
    std::map<space_identifier, space_settings>::iterator It = KWMTiling.SpaceSettings.find(Lookup);
    if(It != KWMTiling.SpaceSettings.end())
        return &It->second;
    else
        return NULL;
}

int GetSpaceFromName(ax_display *Display, std::string Name)
{
    std::map<CGSSpaceID, ax_space>::iterator It;
    for(It = Display->Spaces.begin(); It != Display->Spaces.end(); ++It)
    {
        ax_space *Space = &It->second;
        space_info *SpaceInfo = &WindowTree[Space->Identifier];
        if(SpaceInfo->Settings.Name == Name)
            return Space->ID;
    }

    return -1;
}

void SetNameOfActiveSpace(ax_display *Display, std::string Name)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo) SpaceInfo->Settings.Name = Name;
}

std::string GetNameOfSpace(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    std::string Result = "[no tag]";

    if(!SpaceInfo->Settings.Name.empty())
        Result = SpaceInfo->Settings.Name;

    return Result;
}

void ActivateSpaceWithoutTransition(std::string SpaceID)
{
    ax_display *Display = AXLibMainDisplay();
    if(Display)
    {
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

        if(DestinationSpaceID != ActiveSpace &&
           DestinationSpaceID > 0 && DestinationSpaceID <= TotalSpaces)
        {
            int CGSSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, DestinationSpaceID);
            AXLibSpaceTransition(Display, CGSSpaceID);
        }
    }
}

void MoveWindowBetweenSpaces(ax_display *Display, int SourceSpaceID, int DestinationSpaceID, ax_window *Window)
{
    int SourceCGSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, SourceSpaceID);
    int DestinationCGSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, DestinationSpaceID);
    RemoveWindowFromNodeTree(Display, Window->ID);
    AXLibAddFlags(Window, AXWindow_Minimized);
    ax_space *Space = &Display->Spaces[DestinationCGSpaceID];
    AXLibAddFlags(Space, AXSpace_NeedsUpdate);
    AXLibSpaceRemoveWindow(SourceCGSpaceID, Window->ID);
    AXLibSpaceAddWindow(DestinationCGSpaceID, Window->ID);
}

void MoveFocusedWindowToSpace(std::string SpaceID)
{
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

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

    MoveWindowBetweenSpaces(Display, ActiveSpace, DestinationSpaceID, Window);
}
