#ifndef QUERY_H
#define QUERY_H

#include "types.h"
#include "window.h"
#include "space.h"

#include "axlib/application.h"
#include "axlib/window.h"

extern std::map<CFStringRef, space_info> WindowTree;
extern ax_window *MarkedWindow;

extern int GetNumberOfSpacesOfDisplay(screen_info *Screen);
extern int GetSpaceNumberFromCGSpaceID(screen_info *Screen, int SpaceID);
extern int GetCGSpaceIDFromSpaceNumber(screen_info *Screen, int SpaceID);

extern kwm_mode KWMMode;
extern kwm_toggles KWMToggles;
extern kwm_screen KWMScreen;
extern kwm_tiling KWMTiling;
extern kwm_focus KWMFocus;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

inline std::string
GetActiveTilingMode()
{
    std::string Output;

    if(KWMMode.Space == SpaceModeBSP)
        Output = "bsp";
    else if(KWMMode.Space == SpaceModeMonocle)
        Output = "monocle";
    else
        Output = "float";

    return Output;
}

inline std::string
GetActiveSplitMode()
{
    std::string Output;

    if(KWMScreen.SplitMode == SPLIT_OPTIMAL)
        Output = "Optimal";
    else if(KWMScreen.SplitMode == SPLIT_VERTICAL)
        Output = "Vertical";
    else if(KWMScreen.SplitMode == SPLIT_HORIZONTAL)
        Output = "Horizontal";

    return Output;
}

inline std::string
GetActiveSplitRatio()
{
    std::string Output = std::to_string(KWMScreen.SplitRatio);
    Output.erase(Output.find_last_not_of('0') + 1, std::string::npos);
    return Output;
}

inline std::string
GetActiveSpawnPosition()
{
    return KWMTiling.SpawnAsLeftChild ? "left" : "right";
}

inline std::string
GetStateOfFocusFollowsMouse()
{
    std::string Output;

    if(KWMMode.Focus == FocusModeAutoraise)
        Output = "autoraise";
    else if(KWMMode.Focus == FocusModeDisabled)
        Output = "off";

    return Output;
}

inline std::string
GetStateOfMouseFollowsFocus()
{
    return KWMToggles.UseMouseFollowsFocus ? "on" : "off";
}

inline std::string
GetStateOfCycleFocus()
{
    return KWMMode.Cycle == CycleModeScreen ? "screen" : "off";
}

inline std::string
GetStateOfFloatNonResizable()
{
    return KWMTiling.FloatNonResizable ? "on" : "off";
}

inline std::string
GetStateOfLockToContainer()
{
    return KWMTiling.LockToContainer ? "on" : "off";
}

inline std::string
GetStateOfStandbyOnFloat()
{
    return KWMToggles.StandbyOnFloat ? "on" : "off";
}

/* TODO(koekeishiya): Fix this. */
inline std::string
GetWindowList()
{
    std::string Output;
    /*
    std::vector<window_info> Windows = FilterWindowListAllDisplays();

    for(int Index = 0; Index < Windows.size(); ++Index)
    {
        Output += std::to_string(Windows[Index].WID) + ", " + Windows[Index].Owner + ", " + Windows[Index].Name;
        if(Index < Windows.size() - 1)
            Output += "\n";
    }
    */

    return Output;
}

inline std::string
GetListOfSpaces()
{
    std::string Output;
    screen_info *Screen = KWMScreen.Current;

    if(Screen)
    {
        int SubtractIndex = 0;
        int TotalSpaces = GetNumberOfSpacesOfDisplay(Screen);
        for(int SpaceID = 1; SpaceID <= TotalSpaces; ++SpaceID)
        {
            int CGSpaceID = GetCGSpaceIDFromSpaceNumber(Screen, SpaceID);
            std::map<int, space_info>::iterator It = Screen->Space.find(CGSpaceID);
            if(It != Screen->Space.end())
            {
                if(It->second.Managed)
                {
                    std::string Name = GetNameOfSpace(Screen, CGSpaceID);
                    Output += std::to_string(SpaceID - SubtractIndex) + ", " + Name;
                    if(SpaceID < TotalSpaces && SpaceID < Screen->Space.size())
                        Output += "\n";
                }
                else
                {
                    ++SubtractIndex;
                }
            }
        }

        if(Output[Output.size()-1] == '\n')
            Output.erase(Output.begin() + Output.size()-1);
    }

    return Output;
}

inline std::string
GetNameOfCurrentSpace()
{
    std::string Output;

    ax_display *Display = AXLibMainDisplay();
    Output = GetNameOfSpace(Display);

    return Output;
}

inline std::string
GetNameOfPreviousSpace()
{
    std::string Output;
    if(KWMScreen.Current && !KWMScreen.Current->History.empty())
        Output = GetNameOfSpace(KWMScreen.Current, KWMScreen.Current->History.top());

    return Output;
}

inline std::string
GetModeOfCurrentSpace()
{
    std::string Output;
    GetTagForCurrentSpace(Output);
    return Output;
}

inline std::string
GetTagOfCurrentSpace()
{
    std::string Output;
    GetTagForCurrentSpace(Output);

    ax_application *Application = AXLibGetFocusedApplication();
    if(!Application)
        return Output;
    Output += " " + Application->Name;

    ax_window *Window = Application->Focus;
    if(!Window)
        return Output;

    Output += " - " + Window->Name;
    return Output;
}

inline std::string
GetIdOfCurrentSpace()
{
    std::string Output = "-1";
    if(KWMScreen.Current && KWMScreen.Current->ActiveSpace != -1)
        Output = std::to_string(GetSpaceNumberFromCGSpaceID(KWMScreen.Current, KWMScreen.Current->ActiveSpace));

    return Output;
}

inline std::string
GetIdOfPreviousSpace()
{
    std::string Output = "-1";
    if(KWMScreen.Current && !KWMScreen.Current->History.empty())
        Output = std::to_string(GetSpaceNumberFromCGSpaceID(KWMScreen.Current, KWMScreen.Current->History.top()));

    return Output;
}

inline std::string
GetStateOfFocusedBorder()
{
    return FocusedBorder.Enabled ? "true" : "false";
}

inline std::string
GetStateOfMarkedBorder()
{
    return MarkedBorder.Enabled ? "true" : "false";
}

inline std::string
GetSplitModeOfWindow(ax_window *Window)
{
    std::string Output;
    if(!Window)
        return "";

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return "";

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, Window->ID);
    if(Node)
    {
        if(Node->SplitMode == SPLIT_VERTICAL)
            Output = "Vertical";
        else if(Node->SplitMode == SPLIT_HORIZONTAL)
            Output = "Horizontal";
    }

    return Output;
}

inline std::string
GetIdOfFocusedWindow()
{
    ax_application *Application = AXLibGetFocusedApplication();
    return Application && Application->Focus ? std::to_string(Application->Focus->ID) : "-1";
}

inline std::string
GetNameOfFocusedWindow()
{
    ax_application *Application = AXLibGetFocusedApplication();
    return Application && Application->Focus ? Application->Focus->Name : "";
}

inline std::string
GetSplitModeOfFocusedWindow()
{
    ax_application *Application = AXLibGetFocusedApplication();
    return Application ? GetSplitModeOfWindow(Application->Focus) : "";
}

inline std::string
GetFloatStatusOfFocusedWindow()
{
    ax_application *Application = AXLibGetFocusedApplication();
    return Application && Application->Focus ? (AXLibHasFlags(Application->Focus, AXWindow_Floating) ? "true" : "false") : "false";
}

inline std::string
GetIdOfMarkedWindow()
{
    return MarkedWindow ? std::to_string(MarkedWindow->ID) : "-1";
}

inline std::string
GetNameOfMarkedWindow()
{
    return MarkedWindow ? MarkedWindow->Name : "";
}

inline std::string
GetSplitModeOfMarkedWindow()
{
    return GetSplitModeOfWindow(MarkedWindow);
}

inline std::string
GetFloatStatusOfMarkedWindow()
{
    return MarkedWindow ? (AXLibHasFlags(MarkedWindow, AXWindow_Floating) ? "true" : "false") : "";
}

inline std::string
GetStateOfParentNode(int FirstID, int SecondID)
{
    std::string Output = "false";
    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *FirstNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, FirstID);
        tree_node *SecondNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, SecondID);
        if(FirstNode && SecondNode)
            Output = SecondNode->Parent == FirstNode->Parent ? "true" : "false";
    }

    return Output;
}

inline std::string
GetPositionInNode(int WindowID)
{
    std::string Output;
    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, WindowID);
        if(Node)
            Output = IsLeftChild(Node) ? "left" : "right";
    }

    return Output;
}

inline std::string
GetWindowIdInDirectionOfFocusedWindow(std::string Direction)
{
    ax_window *ClosestWindow = NULL;
    std::string Output = "-1";
    int Degrees = 0;

    if(Direction == "north")
        Degrees = 0;
    else if(Direction == "east")
        Degrees = 90;
    else if(Direction == "south")
        Degrees = 180;
    else if(Direction == "west")
        Degrees = 270;

    if(FindClosestWindow(Degrees, &ClosestWindow, true))
        Output = std::to_string(ClosestWindow->ID);

    return Output;
}

#endif
