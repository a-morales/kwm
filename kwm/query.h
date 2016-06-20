#ifndef QUERY_H
#define QUERY_H

#include "types.h"
#include "window.h"
#include "space.h"

#include "axlib/application.h"
#include "axlib/window.h"

extern std::map<CFStringRef, space_info> WindowTree;
extern ax_window *MarkedWindow;

extern kwm_settings KWMSettings;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

inline std::string
GetActiveTilingMode()
{
    std::string Output;

    if(KWMSettings.Space == SpaceModeBSP)
        Output = "bsp";
    else if(KWMSettings.Space == SpaceModeMonocle)
        Output = "monocle";
    else
        Output = "float";

    return Output;
}

inline std::string
GetActiveSplitMode()
{
    std::string Output;

    if(KWMSettings.SplitMode == SPLIT_OPTIMAL)
        Output = "Optimal";
    else if(KWMSettings.SplitMode == SPLIT_VERTICAL)
        Output = "Vertical";
    else if(KWMSettings.SplitMode == SPLIT_HORIZONTAL)
        Output = "Horizontal";

    return Output;
}

inline std::string
GetActiveSplitRatio()
{
    std::string Output = std::to_string(KWMSettings.SplitRatio);
    Output.erase(Output.find_last_not_of('0') + 1, std::string::npos);
    return Output;
}

inline std::string
GetActiveSpawnPosition()
{
    return KWMSettings.SpawnAsLeftChild ? "left" : "right";
}

inline std::string
GetStateOfFocusFollowsMouse()
{
    std::string Output;

    if(KWMSettings.Focus == FocusModeAutoraise)
        Output = "autoraise";
    else if(KWMSettings.Focus == FocusModeDisabled)
        Output = "off";

    return Output;
}

inline std::string
GetStateOfMouseFollowsFocus()
{
    return KWMSettings.UseMouseFollowsFocus ? "on" : "off";
}

inline std::string
GetStateOfCycleFocus()
{
    return KWMSettings.Cycle == CycleModeScreen ? "screen" : "off";
}

inline std::string
GetStateOfFloatNonResizable()
{
    return KWMSettings.FloatNonResizable ? "on" : "off";
}

inline std::string
GetStateOfLockToContainer()
{
    return KWMSettings.LockToContainer ? "on" : "off";
}

inline std::string
GetStateOfStandbyOnFloat()
{
    return KWMSettings.StandbyOnFloat ? "on" : "off";
}

inline std::string
GetWindowList()
{
    std::string Output;
    std::vector<ax_window *> Windows = AXLibGetAllVisibleWindows();
    for(std::size_t Index = 0; Index < Windows.size(); ++Index)
    {
        ax_window *Window = Windows[Index];
        Output += std::to_string(Window->ID) + ", " + Window->Application->Name;
        if(Window->Name)
            Output +=  ", " + std::string(Window->Name);
        if(Index < Windows.size() - 1)
            Output += "\n";
    }

    return Output;
}

inline std::string
GetListOfSpaces()
{
    std::string Output;
    ax_display *Display = AXLibMainDisplay();
    if(Display)
    {
        int SubtractIndex = 0;
        int TotalSpaces = AXLibDisplaySpacesCount(Display);
        for(int SpaceID = 1; SpaceID <= TotalSpaces; ++SpaceID)
        {
            int CGSSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, SpaceID);
            std::map<int, ax_space>::iterator It = Display->Spaces.find(CGSSpaceID);
            if(It != Display->Spaces.end())
            {
                if(It->second.Type == kCGSSpaceUser)
                {
                    std::string Name = GetNameOfSpace(Display, &It->second);
                    Output += std::to_string(SpaceID - SubtractIndex) + ", " + Name;
                    if(SpaceID < TotalSpaces && SpaceID < Display->Spaces.size())
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
    Output = GetNameOfSpace(Display, Display->Space);

    return Output;
}

inline std::string
GetNameOfPreviousSpace()
{
    std::string Output;
    ax_display *Display = AXLibMainDisplay();
    if(Display)
        Output = GetNameOfSpace(Display, Display->PrevSpace);

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

    if(Window->Name)
        Output += " - " + std::string(Window->Name);

    return Output;
}

inline std::string
GetIdOfCurrentSpace()
{
    std::string Output = "-1";
    ax_display *Display = AXLibMainDisplay();
    if(Display)
        Output = std::to_string(AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID));

    return Output;
}

inline std::string
GetIdOfPreviousSpace()
{
    std::string Output = "-1";
    ax_display *Display = AXLibMainDisplay();
    if(Display)
        Output = std::to_string(AXLibDesktopIDFromCGSSpaceID(Display, Display->PrevSpace->ID));

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
    return MarkedWindow && MarkedWindow->Name ? MarkedWindow->Name : "";
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
    ax_display *Display = AXLibMainDisplay();
    if(!Display)
        return "false";

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    tree_node *FirstNode = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, FirstID);
    tree_node *SecondNode = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, SecondID);
    if(FirstNode && SecondNode)
        Output = SecondNode->Parent == FirstNode->Parent ? "true" : "false";

    return Output;
}

inline std::string
GetPositionInNode(int WindowID)
{
    std::string Output;
    ax_display *Display = AXLibMainDisplay();
    if(!Display)
        return "";

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, WindowID);
    if(Node)
        Output = IsLeftChild(Node) ? "left" : "right";

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
