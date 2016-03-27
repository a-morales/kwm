#include "interpreter.h"
#include "helpers.h"
#include "kwm.h"
#include "daemon.h"
#include "display.h"
#include "space.h"
#include "application.h"
#include "window.h"
#include "container.h"
#include "node.h"
#include "tree.h"
#include "keys.h"
#include "border.h"
#include "serializer.h"
#include "helpers.h"

extern kwm_screen KWMScreen;
extern kwm_toggles KWMToggles;
extern kwm_focus KWMFocus;
extern kwm_mode KWMMode;
extern kwm_tiling KWMTiling;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;
extern kwm_border PrefixBorder;
extern kwm_hotkeys KWMHotkeys;

void KwmConfigCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "reload")
    {
        KwmReloadConfig();
    }
    else if(Tokens[1] == "spaces-key")
    {
        KwmSetSpacesKey(Tokens[2]);
    }
    else if(Tokens[1] == "optimal-ratio")
    {
        KWMTiling.OptimalRatio = ConvertStringToDouble(Tokens[2]);
    }
    else if(Tokens[1] == "prefix-key")
    {
        KwmSetPrefix(Tokens[2]);
    }
    else if(Tokens[1] == "prefix-global")
    {
        if(Tokens[2] == "off")
            KwmSetPrefixGlobal(false);
        else if(Tokens[2] == "on")
            KwmSetPrefixGlobal(true);
    }
    else if(Tokens[1] == "prefix-timeout")
    {
        KwmSetPrefixTimeout(ConvertStringToDouble(Tokens[2]));
    }
    else if(Tokens[1] == "focused-border")
    {
        if(Tokens[2] == "on")
        {
            FocusedBorder.Enabled = true;
            UpdateBorder("focused");
        }
        else if(Tokens[2] == "off")
        {
            FocusedBorder.Enabled = false;
            UpdateBorder("focused");
        }
        else if(Tokens[2] == "size")
        {
            FocusedBorder.Width = ConvertStringToInt(Tokens[3]);
        }
        else if(Tokens[2] == "color")
        {
            FocusedBorder.Color = ConvertHexRGBAToColor(ConvertHexStringToInt(Tokens[3]));
            CreateColorFormat(&FocusedBorder.Color);
        }
    }
    else if(Tokens[1] == "marked-border")
    {
        if(Tokens[2] == "on")
        {
            MarkedBorder.Enabled = true;
        }
        else if(Tokens[2] == "off")
        {
            MarkedBorder.Enabled = false;
            UpdateBorder("marked");
        }
        else if(Tokens[2] == "size")
        {
            MarkedBorder.Width = ConvertStringToInt(Tokens[3]);
        }
        else if(Tokens[2] == "color")
        {
            MarkedBorder.Color = ConvertHexRGBAToColor(ConvertHexStringToInt(Tokens[3]));
            CreateColorFormat(&MarkedBorder.Color);
        }
    }
    else if(Tokens[1] == "prefix-border")
    {
        if(Tokens[2] == "on")
        {
            PrefixBorder.Enabled = true;
        }
        else if(Tokens[2] == "off")
        {
            PrefixBorder.Enabled = false;
            UpdateBorder("focused");
        }
        else if(Tokens[2] == "size")
        {
            PrefixBorder.Width = ConvertStringToInt(Tokens[3]);
        }
        else if(Tokens[2] == "color")
        {
            PrefixBorder.Color = ConvertHexRGBAToColor(ConvertHexStringToInt(Tokens[3]));
            CreateColorFormat(&PrefixBorder.Color);
        }
    }
    else if(Tokens[1] == "float-non-resizable")
    {
        if(Tokens[2] == "off")
            KWMTiling.FloatNonResizable = false;
        else if(Tokens[2] == "on")
            KWMTiling.FloatNonResizable = true;
    }
    else if(Tokens[1] == "lock-to-container")
    {
        if(Tokens[2] == "off")
            KWMTiling.LockToContainer = false;
        else if(Tokens[2] == "on")
            KWMTiling.LockToContainer = true;
    }
    else if(Tokens[1] == "spawn")
    {
        if(Tokens[2] == "left")
            KWMTiling.SpawnAsLeftChild = true;
        else if(Tokens[2] == "right")
            KWMTiling.SpawnAsLeftChild = false;
    }
    else if(Tokens[1] == "tiling")
    {
        if(Tokens[2] == "off")
            KWMToggles.EnableTilingMode = false;
        else if(Tokens[2] == "bsp")
        {
            KWMMode.Space = SpaceModeBSP;
            KWMToggles.EnableTilingMode = true;
        }
        else if(Tokens[2] == "monocle")
        {
            KWMMode.Space = SpaceModeMonocle;
            KWMToggles.EnableTilingMode = true;
        }
        else if(Tokens[2] == "float")
        {
            KWMMode.Space = SpaceModeFloating;
            KWMToggles.EnableTilingMode = true;
        }
    }
    else if(Tokens[1] == "capture")
    {
        CaptureApplicationToScreen(ConvertStringToInt(Tokens[2]), CreateStringFromTokens(Tokens, 3));
    }
    else if(Tokens[1] == "space")
    {
        int ScreenID = ConvertStringToInt(Tokens[2]);
        int DesktopID = ConvertStringToInt(Tokens[3]);
        space_settings *SpaceSettings = GetSpaceSettingsForDesktopID(ScreenID, DesktopID);
        if(!SpaceSettings)
        {
            space_identifier Lookup = { ScreenID, DesktopID };
            space_settings NULLSpaceSettings = { KWMScreen.DefaultOffset, SpaceModeDefault };

            space_settings *ScreenSettings = GetSpaceSettingsForDisplay(ScreenID);
            if(ScreenSettings)
                NULLSpaceSettings = *ScreenSettings;

            KWMTiling.SpaceSettings[Lookup] = NULLSpaceSettings;
            SpaceSettings = &KWMTiling.SpaceSettings[Lookup];
        }

        if(Tokens[4] == "mode")
        {
            if(Tokens[5] == "bsp")
                SpaceSettings->Mode = SpaceModeBSP;
            else if(Tokens[5] == "monocle")
                SpaceSettings->Mode = SpaceModeMonocle;
            else if(Tokens[5] == "float")
                SpaceSettings->Mode = SpaceModeFloating;
        }
        else if(Tokens[4] == "padding")
        {
            SpaceSettings->Offset.PaddingTop = ConvertStringToDouble(Tokens[5]);
            SpaceSettings->Offset.PaddingBottom = ConvertStringToDouble(Tokens[6]);
            SpaceSettings->Offset.PaddingLeft = ConvertStringToDouble(Tokens[7]);
            SpaceSettings->Offset.PaddingRight = ConvertStringToDouble(Tokens[8]);
        }
        else if(Tokens[4] == "gap")
        {
            SpaceSettings->Offset.VerticalGap = ConvertStringToDouble(Tokens[5]);
            SpaceSettings->Offset.HorizontalGap = ConvertStringToDouble(Tokens[6]);
        }
    }
    else if(Tokens[1] == "display")
    {
        int ScreenID = ConvertStringToInt(Tokens[2]);
        space_settings *DisplaySettings = GetSpaceSettingsForDisplay(ScreenID);
        if(!DisplaySettings)
        {
            space_settings NULLSpaceSettings = { KWMScreen.DefaultOffset, SpaceModeDefault };
            KWMTiling.DisplaySettings[ScreenID] = NULLSpaceSettings;
            DisplaySettings = &KWMTiling.DisplaySettings[ScreenID];
        }

        if(Tokens[3] == "mode")
        {
            if(Tokens[4] == "bsp")
                DisplaySettings->Mode = SpaceModeBSP;
            else if(Tokens[4] == "monocle")
                DisplaySettings->Mode = SpaceModeMonocle;
            else if(Tokens[4] == "float")
                DisplaySettings->Mode = SpaceModeFloating;
        }
        else if(Tokens[3] == "padding")
        {
            DisplaySettings->Offset.PaddingTop = ConvertStringToDouble(Tokens[4]);
            DisplaySettings->Offset.PaddingBottom = ConvertStringToDouble(Tokens[5]);
            DisplaySettings->Offset.PaddingLeft = ConvertStringToDouble(Tokens[6]);
            DisplaySettings->Offset.PaddingRight = ConvertStringToDouble(Tokens[7]);
        }
        else if(Tokens[3] == "gap")
        {
            DisplaySettings->Offset.VerticalGap = ConvertStringToDouble(Tokens[4]);
            DisplaySettings->Offset.HorizontalGap = ConvertStringToDouble(Tokens[5]);
        }
    }
    else if(Tokens[1] == "focus")
    {
        if(Tokens[2] == "mouse-follows")
        {
            if(Tokens[3] == "off")
                KWMToggles.UseMouseFollowsFocus = false;
            else if(Tokens[3] == "on")
                KWMToggles.UseMouseFollowsFocus = true;
        }
        else if(Tokens[2] == "standby-on-float")
        {
            if(Tokens[3] == "off")
                KWMToggles.StandbyOnFloat = false;
            else if(Tokens[3] == "on")
                KWMToggles.StandbyOnFloat = true;
        }
        else if(Tokens[2] == "toggle")
        {
            if(KWMMode.Focus == FocusModeDisabled)
                KWMMode.Focus = FocusModeAutofocus;
            else if(KWMMode.Focus == FocusModeAutofocus)
                KWMMode.Focus = FocusModeAutoraise;
            else if(KWMMode.Focus == FocusModeAutoraise)
                KWMMode.Focus = FocusModeDisabled;
        }
        else if(Tokens[2] == "autofocus")
            KWMMode.Focus = FocusModeAutofocus;
        else if(Tokens[2] == "autoraise")
            KWMMode.Focus = FocusModeAutoraise;
        else if(Tokens[2] == "off")
            KWMMode.Focus = FocusModeDisabled;
    }
    else if(Tokens[1] == "cycle-focus")
    {
        if(Tokens[2] == "screen")
            KWMMode.Cycle = CycleModeScreen;
        else if(Tokens[2] == "off")
            KWMMode.Cycle = CycleModeDisabled;;
    }
    else if(Tokens[1] == "hotkeys")
    {
        if(Tokens[2] == "off")
            KWMToggles.UseBuiltinHotkeys = false;
        else if(Tokens[2] == "on")
            KWMToggles.UseBuiltinHotkeys = true;
    }
    else if(Tokens[1] == "float")
    {
        KWMTiling.FloatingAppLst.push_back(CreateStringFromTokens(Tokens, 2));
    }
    else if(Tokens[1] == "add-role")
    {
        AllowRoleForApplication(CreateStringFromTokens(Tokens, 3), Tokens[2]);
    }
    else if(Tokens[1] == "padding")
    {
        container_offset Offset = { ConvertStringToDouble(Tokens[2]),
                                    ConvertStringToDouble(Tokens[3]),
                                    ConvertStringToDouble(Tokens[4]),
                                    ConvertStringToDouble(Tokens[5]),
                                    0,
                                    0
                                  };

        SetDefaultPaddingOfDisplay(Offset);
    }
    else if(Tokens[1] == "gap")
    {
        container_offset Offset = { 0,
                                    0,
                                    0,
                                    0,
                                    ConvertStringToDouble(Tokens[2]),
                                    ConvertStringToDouble(Tokens[3])
                                  };

        SetDefaultGapOfDisplay(Offset);
    }
    else if(Tokens[1] == "split-ratio")
    {
        ChangeSplitRatio(ConvertStringToDouble(Tokens[2]));
    }
}

void KwmQueryCommand(std::vector<std::string> &Tokens, int ClientSockFD)
{
    if(Tokens[1] == "focused")
    {
        std::string Output;
        GetTagForCurrentSpace(Output);

        if(KWMFocus.Window)
            Output += " " + KWMFocus.Window->Owner + (KWMFocus.Window->Name.empty() ? "" : " - " + KWMFocus.Window->Name);

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "current")
    {
        std::string Output = KWMFocus.Window ? std::to_string(KWMFocus.Window->WID) : "-1";
        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "marked")
    {
        std::string Output = std::to_string(KWMScreen.MarkedWindow);;
        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "tag")
    {
        std::string Output;
        GetTagForCurrentSpace(Output);
        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "spawn")
    {
        std::string Output = KWMTiling.SpawnAsLeftChild ? "left" : "right";
        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "prefix")
    {
        std::string Output = KWMHotkeys.Prefix.Active ? "active" : "inactive";
        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "split-ratio")
    {
        std::string Output = std::to_string(KWMScreen.SplitRatio);
        Output.erase(Output.find_last_not_of('0') + 1, std::string::npos);
        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "split-mode")
    {
        std::string Output;
        if(Tokens[2] == "global")
        {
            if(KWMScreen.SplitMode == SPLIT_OPTIMAL)
                Output = "Optimal";
            else if(KWMScreen.SplitMode == SPLIT_VERTICAL)
                Output = "Vertical";
            else if(KWMScreen.SplitMode == SPLIT_HORIZONTAL)
                Output = "Horizontal";
        }
        else
        {
            if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
            {
                int WindowID = ConvertStringToInt(Tokens[2]);
                space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
                tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, WindowID);
                if(Node)
                {
                    if(Node->SplitMode == SPLIT_VERTICAL)
                        Output = "Vertical";
                    else if(Node->SplitMode == SPLIT_HORIZONTAL)
                        Output = "Horizontal";
                }
            }
        }

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "focus")
    {
        std::string Output;
        if(KWMMode.Focus == FocusModeAutofocus)
            Output = "autofocus";
        else if(KWMMode.Focus == FocusModeAutoraise)
            Output = "autoraise";
        else if(KWMMode.Focus == FocusModeDisabled)
            Output = "off";

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "mouse-follows")
    {
        std::string Output;
        if(KWMToggles.UseMouseFollowsFocus)
            Output = "on";
        else
            Output = "off";

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "space")
    {
        std::string Output;
        if(KWMMode.Space == SpaceModeBSP)
            Output = "bsp";
        else if(KWMMode.Space == SpaceModeMonocle)
            Output = "monocle";
        else
            Output = "float";

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "cycle-focus")
    {
        std::string Output;
        if(KWMMode.Cycle == CycleModeScreen)
            Output = "screen";
        else
            Output = "off";

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "border")
    {
        std::string Output = "0";
        if(Tokens[2] == "focused")
            Output = FocusedBorder.Enabled ? "1" : "0";
        else if(Tokens[2] == "marked")
            Output = MarkedBorder.Enabled ? "1" : "0";
        else if(Tokens[2] == "prefix")
            Output = PrefixBorder.Enabled ? "1" : "0";

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "dir")
    {
        window_info Window = {};
        std::string Output = "-1";
        int Degrees = 0;

        if(Tokens[2] == "north")
            Degrees = 0;
        else if(Tokens[2] == "east")
            Degrees = 90;
        else if(Tokens[2] == "south")
            Degrees = 180;
        else if(Tokens[2] == "west")
            Degrees = 270;

        bool Wrap = Tokens[3] == "wrap" ? true : false;
        if(FindClosestWindow(Degrees, &Window, Wrap))
            Output = std::to_string(Window.WID);

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "parent")
    {
        std::string Output = "0";
        if(DoesSpaceExistInMapOfScreen(KWMScreen.Current) && KWMFocus.Window)
        {
            int WindowID = ConvertStringToInt(Tokens[2]);
            space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
            tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, WindowID);
            tree_node *FocusedNode = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);

            if(Node && FocusedNode)
                Output = FocusedNode->Parent == Node->Parent ? "1" : "0";
        }

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "child")
    {
        std::string Output;
        if(DoesSpaceExistInMapOfScreen(KWMScreen.Current) && KWMFocus.Window)
        {
            int WindowID = ConvertStringToInt(Tokens[2]);
            space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
            tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, WindowID);

            if(Node)
                Output = IsLeftChild(Node) ? "left" : "right";
        }

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "windows")
    {
        std::string Output;
        std::vector<window_info> Windows = FilterWindowListAllDisplays();
        for(int Index = 0; Index < Windows.size(); ++Index)
        {
            Output += std::to_string(Windows[Index].WID) + ", " + Windows[Index].Owner + ", " + Windows[Index].Name;
            if(Index < Windows.size() - 1)
                Output += "\n";
        }

        KwmWriteToSocket(ClientSockFD, Output);
    }
}

/* -------------------------------------------------- */
/* NEW COMMANDS - EVERYTHING ABOVE SHOULD BE REVAMPED */
/* -------------------------------------------------- */
void KwmMarkCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "focused")
        {
            MarkFocusedWindowContainer();
            return;
        }

        window_info Window = {};
        std::string Output = "-1";
        int Degrees = 0;

        if(Tokens[2] == "north")
            Degrees = 0;
        else if(Tokens[2] == "east")
            Degrees = 90;
        else if(Tokens[2] == "south")
            Degrees = 180;
        else if(Tokens[2] == "west")
            Degrees = 270;

        bool Wrap = Tokens[3] == "wrap" ? true : false;
        if(FindClosestWindow(Degrees, &Window, Wrap))
            MarkWindowContainer(&Window);
    }
}

void KwmTreeCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "rotate")
    {
        if(Tokens[2] == "90" || Tokens[2] == "270" || Tokens[2] == "180")
        {
            space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
            if(Space->Settings.Mode == SpaceModeBSP)
            {
                RotateTree(Space->RootNode, ConvertStringToInt(Tokens[2]));
                CreateNodeContainers(KWMScreen.Current, Space->RootNode, false);
                ApplyTreeNodeContainer(Space->RootNode);
            }
        }
    }
    else if(Tokens[1] == "save")
    {
        SaveBSPTreeToFile(KWMScreen.Current, Tokens[2]);
    }
    else if(Tokens[1] == "restore")
    {
        LoadBSPTreeFromFile(KWMScreen.Current, Tokens[2]);
    }
}

void KwmFocusCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "north")
            ShiftWindowFocusDirected(0);
        else if(Tokens[2] == "east")
            ShiftWindowFocusDirected(90);
        else if(Tokens[2] == "south")
            ShiftWindowFocusDirected(180);
        else if(Tokens[2] == "west")
            ShiftWindowFocusDirected(270);
        else if(Tokens[2] == "prev")
            ShiftWindowFocus(-1);
        else if(Tokens[2] == "next")
            ShiftWindowFocus(1);
        else if(Tokens[2] == "curr")
            FocusWindowBelowCursor();
        else
            FocusWindowByID(ConvertStringToInt(Tokens[2]));
    }
    else if(Tokens[1] == "-sub-window")
    {
        if(Tokens[2] == "prev")
            ShiftSubTreeWindowFocus(-1);
        else if(Tokens[2] == "next")
            ShiftSubTreeWindowFocus(1);
    }
    else if(Tokens[1] == "-space")
    {
        KwmEmitKeystroke(KWMHotkeys.SpacesKey, Tokens[2]);
    }
    else if(Tokens[1] == "-display")
    {
        if(Tokens[2] == "prev")
            GiveFocusToScreen(GetIndexOfPrevScreen(), NULL, false, true);
        else if(Tokens[2] == "next")
            GiveFocusToScreen(GetIndexOfNextScreen(), NULL, false, true);
        else
            GiveFocusToScreen(ConvertStringToInt(Tokens[2]), NULL, false, true);
    }
}

void KwmSwapCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "north")
            SwapFocusedWindowDirected(0);
        else if(Tokens[2] == "east")
            SwapFocusedWindowDirected(90);
        else if(Tokens[2] == "south")
            SwapFocusedWindowDirected(180);
        else if(Tokens[2] == "west")
            SwapFocusedWindowDirected(270);
        else if(Tokens[2] == "prev")
            SwapFocusedWindowWithNearest(-1);
        else if(Tokens[2] == "next")
            SwapFocusedWindowWithNearest(1);
        else if(Tokens[2] == "mark")
            SwapFocusedWindowWithMarked();
    }
}

void KwmZoomCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "fullscreen")
            ToggleFocusedWindowFullscreen();
        else if(Tokens[2] == "parent")
            ToggleFocusedWindowParentContainer();
    }
}

void KwmFloatCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "focused")
            ToggleFocusedWindowFloating();
    }
    else if(Tokens[1] == "-space")
    {
        if(Tokens[2] == "focused")
            FloatFocusedSpace();
    }
}

void KwmRefreshCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "focused")
            ResizeWindowToContainerSize();
    }
    else if(Tokens[1] == "-space")
    {
        if(Tokens[2] == "focused")
        {
            space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
            ApplyTreeNodeContainer(Space->RootNode);
        }
    }
}

void KwmNodeCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "reduce" || Tokens[2] == "expand")
        {
            double Ratio = ConvertStringToDouble(Tokens[3]);
            if(Tokens[2] == "reduce")
                ModifyContainerSplitRatio(-Ratio);
            else if(Tokens[2] == "expand")
                ModifyContainerSplitRatio(Ratio);
        }
        else if(Tokens[2] == "type")
        {
            if(Tokens[3] == "monocle")
                ChangeTypeOfFocusedNode(NodeTypeLink);
            else if(Tokens[3] == "bsp")
                ChangeTypeOfFocusedNode(NodeTypeTree);
            else if(Tokens[3] == "toggle")
                ToggleTypeOfFocusedNode();
        }
    }
    else if(Tokens[1] == "-pseudo")
    {
        if(Tokens[2] == "create")
            CreatePseudoNode();
        else if(Tokens[2] == "destroy")
            RemovePseudoNode();
    }
}

void KwmMoveCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(!KWMFocus.Window)
            return;

        if(Tokens[2] == "space")
        {
            MoveWindowToSpace(Tokens[3]);
        }
        else if(Tokens[2] == "display")
        {
            if(IsApplicationCapturedByScreen(KWMFocus.Window))
                return;

            if(Tokens[3] == "prev")
                MoveWindowToDisplay(KWMFocus.Window, -1, true, true);
            else if(Tokens[3] == "next")
                MoveWindowToDisplay(KWMFocus.Window, 1, true, true);
            else
                MoveWindowToDisplay(KWMFocus.Window, ConvertStringToInt(Tokens[3]), false, true);
        }
        else if(Tokens[2] == "north")
        {
            DetachAndReinsertWindow(KWMFocus.Window->WID, 0);
        }
        else if(Tokens[2] == "east")
        {
            DetachAndReinsertWindow(KWMFocus.Window->WID, 90);
        }
        else if(Tokens[2] == "south")
        {
            DetachAndReinsertWindow(KWMFocus.Window->WID, 180);
        }
        else if(Tokens[2] == "west")
        {
            DetachAndReinsertWindow(KWMFocus.Window->WID, 270);
        }
        else if(Tokens[2] == "mark")
        {
            DetachAndReinsertWindow(KWMScreen.MarkedWindow, 0);
        }
        else
        {
            int XOff = ConvertStringToInt(Tokens[2]);
            int YOff = ConvertStringToInt(Tokens[3]);
            MoveFloatingWindow(XOff, YOff);
        }
    }
}

void KwmTileCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-space")
    {
        if(Tokens[2] == "bsp")
            TileFocusedSpace(SpaceModeBSP);
        else if(Tokens[2] == "monocle")
            TileFocusedSpace(SpaceModeMonocle);
    }
}

void KwmSplitCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-window")
    {
        if(Tokens[2] == "toggle")
        {
            space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
            tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);

            if(Node)
                ToggleNodeSplitMode(KWMScreen.Current, Node->Parent);
        }
    }
    else if(Tokens[1] == "-display")
    {
        if(Tokens[2] == "optimal")
            KWMScreen.SplitMode = SPLIT_OPTIMAL;
        else if(Tokens[2] == "vertical")
            KWMScreen.SplitMode = SPLIT_VERTICAL;
        else if(Tokens[2] == "horizontal")
            KWMScreen.SplitMode = SPLIT_HORIZONTAL;
    }
}

void KwmPaddingCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-space")
    {
        if(Tokens[3] == "left" || Tokens[3] == "right" ||
           Tokens[3] == "top" || Tokens[3] == "bottom" ||
           Tokens[3] == "all")
        {
            int Value = 0;
            if(Tokens[2] == "increase")
                Value = 10;
            else if(Tokens[2] == "decrease")
                Value = -10;

            ChangePaddingOfDisplay(Tokens[3], Value);
        }
    }
}

void KwmGapCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-space")
    {
        if(Tokens[3] == "vertical" || Tokens[3] == "horizontal" ||
           Tokens[3] == "all")
        {
            int Value = 0;
            if(Tokens[2] == "increase")
                Value = 10;
            else if(Tokens[2] == "decrease")
                Value = -10;

            ChangeGapOfDisplay(Tokens[3], Value);
        }
    }
}

void KwmBindCommand(std::vector<std::string> &Tokens, bool Passthrough)
{
    if(Tokens.size() > 2)
        KwmAddHotkey(Tokens[1], CreateStringFromTokens(Tokens, 2), Passthrough);
    else
        KwmAddHotkey(Tokens[1], "", Passthrough);
}

void KwmInterpretCommand(std::string Message, int ClientSockFD)
{
    std::vector<std::string> Tokens = SplitString(Message, ' ');

    if(Tokens[0] == "quit")
        KwmQuit();
    else if(Tokens[0] == "config")
        KwmConfigCommand(Tokens);
    else if(Tokens[0] == "query")
        KwmQueryCommand(Tokens, ClientSockFD);
    else if(Tokens[0] == "focus")
        KwmFocusCommand(Tokens);
    else if(Tokens[0] == "swap")
        KwmSwapCommand(Tokens);
    else if(Tokens[0] == "zoom")
        KwmZoomCommand(Tokens);
    else if(Tokens[0] == "float")
        KwmFloatCommand(Tokens);
    else if(Tokens[0] == "refresh")
        KwmRefreshCommand(Tokens);
    else if(Tokens[0] == "node")
        KwmNodeCommand(Tokens);
    else if(Tokens[0] == "move")
        KwmMoveCommand(Tokens);
    else if(Tokens[0] == "tile")
        KwmTileCommand(Tokens);
    else if(Tokens[0] == "split")
        KwmSplitCommand(Tokens);
    else if(Tokens[0] == "padding")
        KwmPaddingCommand(Tokens);
    else if(Tokens[0] == "gap")
        KwmGapCommand(Tokens);
    else if(Tokens[0] == "mark")
        KwmMarkCommand(Tokens);
    else if(Tokens[0] == "tree")
        KwmTreeCommand(Tokens);
    else if(Tokens[0] == "write")
        KwmEmitKeystrokes(CreateStringFromTokens(Tokens, 1));
    else if(Tokens[0] == "press")
        KwmEmitKeystroke(Tokens[1]);
    else if(Tokens[0] == "bind")
        KwmBindCommand(Tokens, false);
    else if(Tokens[0] == "bind-passthrough")
        KwmBindCommand(Tokens, true);
    else if(Tokens[0] == "unbind")
        KwmRemoveHotkey(Tokens[1]);
}
