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
#include "rules.h"
#include "query.h"
#include "scratchpad.h"

#define internal static

extern kwm_screen KWMScreen;
extern kwm_toggles KWMToggles;
extern kwm_focus KWMFocus;
extern kwm_mode KWMMode;
extern kwm_tiling KWMTiling;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;
extern kwm_hotkeys KWMHotkeys;

void MoveFocusedWindowToSpace(std::string SpaceID);
void ActivateSpaceWithoutTransition(std::string SpaceID);

internal void
KwmConfigCommand(std::vector<std::string> &Tokens)
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
    else if(Tokens[1] == "border")
    {
        if(Tokens[2] == "focused")
        {
            if(Tokens[3] == "on")
            {
                FocusedBorder.Enabled = true;
                UpdateBorder("focused");
            }
            else if(Tokens[3] == "off")
            {
                FocusedBorder.Enabled = false;
                KWMTiling.KwmOverlay[0] = 0;
                UpdateBorder("focused");
            }
            else if(Tokens[3] == "size")
            {
                FocusedBorder.Width = ConvertStringToInt(Tokens[4]);
            }
            else if(Tokens[3] == "color")
            {
                FocusedBorder.Color = ConvertHexRGBAToColor(ConvertHexStringToInt(Tokens[4]));
                CreateColorFormat(&FocusedBorder.Color);
                mode *BindingMode = GetBindingMode("default");
                BindingMode->Color = FocusedBorder.Color;
            }
            else if(Tokens[3] == "radius")
            {
                FocusedBorder.Radius = ConvertStringToDouble(Tokens[4]);
            }
        }
        else if(Tokens[2] == "marked")
        {
            if(Tokens[3] == "on")
            {
                MarkedBorder.Enabled = true;
            }
            else if(Tokens[3] == "off")
            {
                MarkedBorder.Enabled = false;
                KWMTiling.KwmOverlay[1] = 0;
                UpdateBorder("marked");
            }
            else if(Tokens[3] == "size")
            {
                MarkedBorder.Width = ConvertStringToInt(Tokens[4]);
            }
            else if(Tokens[3] == "color")
            {
                MarkedBorder.Color = ConvertHexRGBAToColor(ConvertHexStringToInt(Tokens[4]));
                CreateColorFormat(&MarkedBorder.Color);
            }
            else if(Tokens[3] == "radius")
            {
                MarkedBorder.Radius = ConvertStringToDouble(Tokens[4]);
            }
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
    else if(Tokens[1] == "space")
    {
        int ScreenID = ConvertStringToInt(Tokens[2]);
        int DesktopID = ConvertStringToInt(Tokens[3]);
        space_settings *SpaceSettings = GetSpaceSettingsForDesktopID(ScreenID, DesktopID);
        if(!SpaceSettings)
        {
            space_identifier Lookup = { ScreenID, DesktopID };
            space_settings NULLSpaceSettings = { KWMScreen.DefaultOffset, SpaceModeDefault, "", ""};

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
        else if(Tokens[4] == "name")
        {
            SpaceSettings->Name = Tokens[5];
        }
        else if(Tokens[4] == "tree")
        {
            SpaceSettings->Layout = Tokens[5];
        }
    }
    else if(Tokens[1] == "display")
    {
        int ScreenID = ConvertStringToInt(Tokens[2]);
        space_settings *DisplaySettings = GetSpaceSettingsForDisplay(ScreenID);
        if(!DisplaySettings)
        {
            space_settings NULLSpaceSettings = { KWMScreen.DefaultOffset, SpaceModeDefault, "", "" };
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
    else if(Tokens[1] == "focus-follows-mouse")
    {
        if(Tokens[2] == "toggle")
        {
            if(KWMMode.Focus == FocusModeDisabled)
                KWMMode.Focus = FocusModeAutoraise;
            else if(KWMMode.Focus == FocusModeAutoraise)
                KWMMode.Focus = FocusModeDisabled;
        }
        else if(Tokens[2] == "autoraise")
            KWMMode.Focus = FocusModeAutoraise;
        else if(Tokens[2] == "off")
            KWMMode.Focus = FocusModeDisabled;
    }
    else if(Tokens[1] == "mouse-follows-focus")
    {
        if(Tokens[2] == "off")
            KWMToggles.UseMouseFollowsFocus = false;
        else if(Tokens[2] == "on")
            KWMToggles.UseMouseFollowsFocus = true;
    }
    else if(Tokens[1] == "standby-on-float")
    {
        if(Tokens[2] == "off")
            KWMToggles.StandbyOnFloat = false;
        else if(Tokens[2] == "on")
            KWMToggles.StandbyOnFloat = true;
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

internal void
KwmQueryCommand(std::vector<std::string> &Tokens, int ClientSockFD)
{
    if(Tokens[1] == "tiling")
    {
        if(Tokens[2] == "mode")
            KwmWriteToSocket(ClientSockFD, GetActiveTilingMode());
        else if(Tokens[2] == "spawn")
            KwmWriteToSocket(ClientSockFD, GetActiveSpawnPosition());
        else if(Tokens[2] == "split-mode")
            KwmWriteToSocket(ClientSockFD, GetActiveSplitMode());
        else if(Tokens[2] == "split-ratio")
            KwmWriteToSocket(ClientSockFD, GetActiveSplitRatio());
    }
    else if(Tokens[1] == "window")
    {
        if(Tokens[2] == "focused")
        {
            if(Tokens[3] == "id")
                KwmWriteToSocket(ClientSockFD, GetIdOfFocusedWindow());
            else if(Tokens[3] == "name")
                KwmWriteToSocket(ClientSockFD, GetNameOfFocusedWindow());
            else if(Tokens[3] == "split")
                KwmWriteToSocket(ClientSockFD, GetSplitModeOfFocusedWindow());
            else if(Tokens[3] == "float")
                KwmWriteToSocket(ClientSockFD, GetFloatStatusOfFocusedWindow());
            else
                KwmWriteToSocket(ClientSockFD, GetWindowIdInDirectionOfFocusedWindow(Tokens[3]));
        }
        else if(Tokens[2] == "marked")
        {
            if(Tokens[3] == "id")
                KwmWriteToSocket(ClientSockFD, GetIdOfMarkedWindow());
            else if(Tokens[3] == "name")
                KwmWriteToSocket(ClientSockFD, GetNameOfMarkedWindow());
            else if(Tokens[3] == "split")
                KwmWriteToSocket(ClientSockFD, GetSplitModeOfMarkedWindow());
            else if(Tokens[3] == "float")
                KwmWriteToSocket(ClientSockFD, GetFloatStatusOfMarkedWindow());
        }
        else if(Tokens[2] == "parent")
        {
            int FirstID = ConvertStringToInt(Tokens[3]);
            int SecondID = ConvertStringToInt(Tokens[4]);
            KwmWriteToSocket(ClientSockFD, GetStateOfParentNode(FirstID, SecondID));
        }
        else if(Tokens[2] == "child")
        {
            int WindowID = ConvertStringToInt(Tokens[3]);
            KwmWriteToSocket(ClientSockFD, GetPositionInNode(WindowID));
        }
        else if(Tokens[2] == "list")
        {
            KwmWriteToSocket(ClientSockFD, GetWindowList());
        }
    }
    else if(Tokens[1] == "space")
    {
        if(Tokens[2] == "active")
        {
            if(Tokens[3] == "tag")
                KwmWriteToSocket(ClientSockFD, GetTagOfCurrentSpace());
            else if(Tokens[3] == "name")
                KwmWriteToSocket(ClientSockFD, GetNameOfCurrentSpace());
            else if(Tokens[3] == "id")
                KwmWriteToSocket(ClientSockFD, GetIdOfCurrentSpace());
            else if(Tokens[3] == "mode")
                KwmWriteToSocket(ClientSockFD, GetModeOfCurrentSpace());
        }
        else if(Tokens[2] == "previous")
        {
            if(Tokens[3] == "name")
                KwmWriteToSocket(ClientSockFD, GetNameOfPreviousSpace());
            else if(Tokens[3] == "id")
                KwmWriteToSocket(ClientSockFD, GetIdOfPreviousSpace());
        }
        else if(Tokens[2] == "list")
            KwmWriteToSocket(ClientSockFD, GetListOfSpaces());
    }
    else if(Tokens[1] == "border")
    {
        if(Tokens[2] == "focused")
            KwmWriteToSocket(ClientSockFD, GetStateOfFocusedBorder());
        else if(Tokens[2] == "marked")
            KwmWriteToSocket(ClientSockFD, GetStateOfMarkedBorder());
    }
    else if(Tokens[1] == "cycle-focus")
    {
        KwmWriteToSocket(ClientSockFD, GetStateOfCycleFocus());
    }
    else if(Tokens[1] == "float-non-resizable")
    {
        KwmWriteToSocket(ClientSockFD, GetStateOfFloatNonResizable());
    }
    else if(Tokens[1] == "lock-to-container")
    {
        KwmWriteToSocket(ClientSockFD, GetStateOfLockToContainer());
    }
    else if(Tokens[1] == "standby-on-float")
    {
        KwmWriteToSocket(ClientSockFD, GetStateOfStandbyOnFloat());
    }
    else if(Tokens[1] == "focus-follows-mouse")
    {
        KwmWriteToSocket(ClientSockFD, GetStateOfFocusFollowsMouse());
    }
    else if(Tokens[1] == "mouse-follows-focus")
    {
        KwmWriteToSocket(ClientSockFD, GetStateOfMouseFollowsFocus());
    }
}

internal void
KwmModeCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "activate")
        KwmActivateBindingMode(Tokens[2]);
    else
    {
        std::string Mode = Tokens[1];
        mode *BindingMode = GetBindingMode(Mode);
        if(Tokens[2] == "color")
        {
            BindingMode->Color = ConvertHexRGBAToColor(ConvertHexStringToInt(Tokens[3]));
            CreateColorFormat(&BindingMode->Color);
        }
        else if(Tokens[2] == "prefix")
        {
            if(Tokens[3] == "on")
                BindingMode->Prefix = true;
            else if(Tokens[3] == "off")
                BindingMode->Prefix = false;
        }
        else if(Tokens[2] == "timeout")
        {
            BindingMode->Timeout = ConvertStringToDouble(Tokens[3]);
        }
        else if(Tokens[2] == "restore")
        {
            BindingMode->Restore = Tokens[3];
        }
    }
}

internal void
KwmBindCommand(std::vector<std::string> &Tokens, bool Passthrough)
{
    bool BindCode = Tokens[0].find("bindcode") != std::string::npos;

    if(Tokens.size() > 2)
        KwmAddHotkey(Tokens[1], CreateStringFromTokens(Tokens, 2), Passthrough, BindCode);
    else
        KwmAddHotkey(Tokens[1], "", Passthrough, BindCode);
}

internal void
KwmWindowCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-f")
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
            FocusWindowByID(ConvertStringToUint(Tokens[2]));
    }
    else if(Tokens[1] == "-fm")
    {
        if(Tokens[2] == "prev")
            ShiftSubTreeWindowFocus(-1);
        else if(Tokens[2] == "next")
            ShiftSubTreeWindowFocus(1);
    }
    else if(Tokens[1] == "-s")
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
    else if(Tokens[1] == "-z")
    {
        if(Tokens[2] == "fullscreen")
            ToggleFocusedWindowFullscreen();
        else if(Tokens[2] == "parent")
            ToggleFocusedWindowParentContainer();
    }
    else if(Tokens[1] == "-t")
    {
        if(Tokens[2] == "focused")
            ToggleFocusedWindowFloating();
    }
    else if(Tokens[1] == "-r")
    {
        if(Tokens[2] == "focused")
            ResizeWindowToContainerSize();
    }
    else if(Tokens[1] == "-c")
    {
        if(Tokens[2] == "split-mode")
        {
            if(Tokens[3] == "toggle")
            {
                space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
                tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);

                if(Node)
                    ToggleNodeSplitMode(KWMScreen.Current, Node->Parent);
            }
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
        else if(Tokens[2] == "reduce" || Tokens[2] == "expand")
        {
            double Ratio = ConvertStringToDouble(Tokens[3]);
            Ratio = Tokens[2] == "reduce" ? -Ratio : Ratio;

            if(Tokens.size() == 5)
            {
                if(Tokens[4] == "north")
                    ModifyContainerSplitRatio(Ratio, 0);
                else if(Tokens[4] == "east")
                    ModifyContainerSplitRatio(Ratio, 90);
                else if(Tokens[4] == "south")
                    ModifyContainerSplitRatio(Ratio, 180);
                else if(Tokens[4] == "west")
                    ModifyContainerSplitRatio(Ratio, 270);
            }
            else
            {
                ModifyContainerSplitRatio(Ratio);
            }
        }
    }
    else if(Tokens[1] == "-m")
    {
        if(!KWMFocus.Window)
            return;

        if(Tokens[2] == "space")
        {
            if(Tokens[3] == "previous")
                GoToPreviousSpace(true);
            else
                MoveFocusedWindowToSpace(Tokens[3]);
        }
        else if(Tokens[2] == "display")
        {
            if(Tokens[3] == "prev")
                MoveWindowToDisplay(KWMFocus.Window, -1, true);
            else if(Tokens[3] == "next")
                MoveWindowToDisplay(KWMFocus.Window, 1, true);
            else
                MoveWindowToDisplay(KWMFocus.Window, ConvertStringToInt(Tokens[3]), false);
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
            DetachAndReinsertWindow(KWMScreen.MarkedWindow.WID, 0);
        }
        else
        {
            int XOff = ConvertStringToInt(Tokens[2]);
            int YOff = ConvertStringToInt(Tokens[3]);
            MoveFloatingWindow(XOff, YOff);
        }
    }
    else if(Tokens[1] == "-mk")
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

internal void
KwmSpaceCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-f")
    {
        if(Tokens[2] == "previous")
            GoToPreviousSpace(false);
        else
            KwmEmitKeystroke(KWMHotkeys.SpacesKey, Tokens[2]);
    }
    else if(Tokens[1] == "-fExperimental")
    {
        ActivateSpaceWithoutTransition(Tokens[2]);
    }
    else if(Tokens[1] == "-t")
    {
        if(Tokens[2] == "bsp")
            TileFocusedSpace(SpaceModeBSP);
        else if(Tokens[2] == "monocle")
            TileFocusedSpace(SpaceModeMonocle);
        else if(Tokens[2] == "float")
            FloatFocusedSpace();
    }
    else if(Tokens[1] == "-r")
    {
        if(Tokens[2] == "focused")
        {
            space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
            ApplyTreeNodeContainer(Space->RootNode);
        }
    }
    else if(Tokens[1] == "-p")
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
    else if(Tokens[1] == "-g")
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
    else if(Tokens[1] == "-n")
    {
        if(KWMScreen.Current &&
           KWMScreen.Current->ActiveSpace != -1)
            SetNameOfActiveSpace(KWMScreen.Current, Tokens[2]);
    }
}

internal void
KwmDisplayCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-f")
    {
        if(Tokens[2] == "prev")
            GiveFocusToScreen(GetIndexOfPrevScreen(), NULL, false, true);
        else if(Tokens[2] == "next")
            GiveFocusToScreen(GetIndexOfNextScreen(), NULL, false, true);
        else
            GiveFocusToScreen(ConvertStringToInt(Tokens[2]), NULL, false, true);
    }
    else if(Tokens[1] == "-c")
    {
        if(Tokens[2] == "optimal")
            KWMScreen.SplitMode = SPLIT_OPTIMAL;
        else if(Tokens[2] == "vertical")
            KWMScreen.SplitMode = SPLIT_VERTICAL;
        else if(Tokens[2] == "horizontal")
            KWMScreen.SplitMode = SPLIT_HORIZONTAL;
    }
}

internal void
KwmTreeCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-pseudo")
    {
        if(Tokens[2] == "create")
            CreatePseudoNode();
        else if(Tokens[2] == "destroy")
            RemovePseudoNode();
    }
    else if(Tokens[1] == "rotate")
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

internal void
KwmScratchpadCommand(std::vector<std::string> &Tokens, int ClientSockFD)
{
    if(Tokens[1] == "show")
        ShowScratchpadWindow(ConvertStringToInt(Tokens[2]));
    else if(Tokens[1] == "toggle")
        ToggleScratchpadWindow(ConvertStringToInt(Tokens[2]));
    else if(Tokens[1] == "hide")
        HideScratchpadWindow(ConvertStringToInt(Tokens[2]));
    else if(Tokens[1] == "add" && KWMFocus.Window)
        AddWindowToScratchpad(KWMFocus.Window);
    else if(Tokens[1] == "remove" && KWMFocus.Window)
        RemoveWindowFromScratchpad(KWMFocus.Window);
    else if(Tokens[1] == "list")
        KwmWriteToSocket(ClientSockFD, GetWindowsOnScratchpad());
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
    else if(Tokens[0] == "window")
        KwmWindowCommand(Tokens);
    else if(Tokens[0] == "space")
        KwmSpaceCommand(Tokens);
    else if(Tokens[0] == "display")
        KwmDisplayCommand(Tokens);
    else if(Tokens[0] == "tree")
        KwmTreeCommand(Tokens);
    else if(Tokens[0] == "write")
        KwmEmitKeystrokes(CreateStringFromTokens(Tokens, 1));
    else if(Tokens[0] == "press")
        KwmEmitKeystroke(Tokens[1]);
    else if(Tokens[0] == "mode")
        KwmModeCommand(Tokens);
    else if(Tokens[0] == "bindsym" || Tokens[0] == "bindcode")
        KwmBindCommand(Tokens, false);
    else if(Tokens[0] == "bindsym_passthrough" || Tokens[0] == "bindcode_passthrough")
        KwmBindCommand(Tokens, true);
    else if(Tokens[0] == "unbindsym" || Tokens[0] == "unbindcode")
        KwmRemoveHotkey(Tokens[1], Tokens[0] == "unbindcode");
    else if(Tokens[0] == "rule")
        KwmAddRule(CreateStringFromTokens(Tokens, 1));
    else if(Tokens[0] == "scratchpad")
        KwmScratchpadCommand(Tokens, ClientSockFD);
}
