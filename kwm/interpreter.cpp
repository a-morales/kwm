#include "interpreter.h"
#include "helpers.h"
#include "kwm.h"
#include "daemon.h"
#include "display.h"
#include "space.h"
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
#include "cursor.h"

#define internal static

extern std::map<CFStringRef, space_info> WindowTree;
extern ax_application *FocusedApplication;
extern ax_window *MarkedWindow;

extern kwm_settings KWMSettings;;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;
extern kwm_hotkeys KWMHotkeys;

internal void
KwmConfigCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "reload")
    {
        KwmReloadConfig();
    }
    else if(Tokens[1] == "optimal-ratio")
    {
        KWMSettings.OptimalRatio = ConvertStringToDouble(Tokens[2]);
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
            KWMSettings.FloatNonResizable = false;
        else if(Tokens[2] == "on")
            KWMSettings.FloatNonResizable = true;
    }
    else if(Tokens[1] == "lock-to-container")
    {
        if(Tokens[2] == "off")
            KWMSettings.LockToContainer = false;
        else if(Tokens[2] == "on")
            KWMSettings.LockToContainer = true;
    }
    else if(Tokens[1] == "spawn")
    {
        if(Tokens[2] == "left")
            KWMSettings.SpawnAsLeftChild = true;
        else if(Tokens[2] == "right")
            KWMSettings.SpawnAsLeftChild = false;
    }
    else if(Tokens[1] == "tiling")
    {
        if(Tokens[2] == "bsp")
            KWMSettings.Space = SpaceModeBSP;
        else if(Tokens[2] == "monocle")
            KWMSettings.Space = SpaceModeMonocle;
        else if(Tokens[2] == "float")
            KWMSettings.Space = SpaceModeFloating;
    }
    else if(Tokens[1] == "space")
    {
        int ScreenID = ConvertStringToInt(Tokens[2]);
        int DesktopID = ConvertStringToInt(Tokens[3]);
        space_settings *SpaceSettings = GetSpaceSettingsForDesktopID(ScreenID, DesktopID);
        if(!SpaceSettings)
        {
            space_identifier Lookup = { ScreenID, DesktopID };
            space_settings NULLSpaceSettings = { KWMSettings.DefaultOffset, SpaceModeDefault, "", ""};

            space_settings *ScreenSettings = GetSpaceSettingsForDisplay(ScreenID);
            if(ScreenSettings)
                NULLSpaceSettings = *ScreenSettings;

            KWMSettings.SpaceSettings[Lookup] = NULLSpaceSettings;
            SpaceSettings = &KWMSettings.SpaceSettings[Lookup];
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
            space_settings NULLSpaceSettings = { KWMSettings.DefaultOffset, SpaceModeDefault, "", "" };
            KWMSettings.DisplaySettings[ScreenID] = NULLSpaceSettings;
            DisplaySettings = &KWMSettings.DisplaySettings[ScreenID];
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
            if(KWMSettings.Focus == FocusModeDisabled)
                KWMSettings.Focus = FocusModeAutoraise;
            else if(KWMSettings.Focus == FocusModeAutoraise)
                KWMSettings.Focus = FocusModeDisabled;
        }
        else if(Tokens[2] == "on")
            KWMSettings.Focus = FocusModeAutoraise;
        else if(Tokens[2] == "off")
            KWMSettings.Focus = FocusModeDisabled;
    }
    else if(Tokens[1] == "mouse-follows-focus")
    {
        if(Tokens[2] == "off")
            KWMSettings.UseMouseFollowsFocus = false;
        else if(Tokens[2] == "on")
            KWMSettings.UseMouseFollowsFocus = true;
    }
    else if(Tokens[1] == "standby-on-float")
    {
        if(Tokens[2] == "off")
            KWMSettings.StandbyOnFloat = false;
        else if(Tokens[2] == "on")
            KWMSettings.StandbyOnFloat = true;
    }
    else if(Tokens[1] == "cycle-focus")
    {
        if(Tokens[2] == "on")
            KWMSettings.Cycle = CycleModeScreen;
        else if(Tokens[2] == "off")
            KWMSettings.Cycle = CycleModeDisabled;;
    }
    else if(Tokens[1] == "hotkeys")
    {
        if(Tokens[2] == "off")
            KWMSettings.UseBuiltinHotkeys = false;
        else if(Tokens[2] == "on")
            KWMSettings.UseBuiltinHotkeys = true;
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
        else if(Tokens[2] == "first")
            FocusFirstLeafNode(AXLibCursorDisplay());
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
                ToggleFocusedNodeSplitMode();
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
        if(Tokens[2] == "space")
        {
            if(Tokens[3] == "previous")
                GoToPreviousSpace(true);
            else
                MoveFocusedWindowToSpace(Tokens[3]);
        }
        else if(Tokens[2] == "display")
        {
            ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
            if(!Window)
                return;

            if(Tokens[3] == "prev")
                MoveWindowToDisplay(Window, -1, true);
            else if(Tokens[3] == "next")
                MoveWindowToDisplay(Window, 1, true);
            else
                MoveWindowToDisplay(Window, ConvertStringToInt(Tokens[3]), false);
        }
        else if(Tokens[2] == "north")
        {
            ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
            if(Window)
                DetachAndReinsertWindow(Window->ID, 0);
        }
        else if(Tokens[2] == "east")
        {
            ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
            if(Window)
                DetachAndReinsertWindow(Window->ID, 90);
        }
        else if(Tokens[2] == "south")
        {
            ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
            if(Window)
                DetachAndReinsertWindow(Window->ID, 180);
        }
        else if(Tokens[2] == "west")
        {
            ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
            if(Window)
                DetachAndReinsertWindow(Window->ID, 270);
        }
        else if(Tokens[2] == "mark")
        {
            if(MarkedWindow)
                DetachAndReinsertWindow(MarkedWindow->ID, 0);
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

        ax_window *ClosestWindow = NULL;
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
        if((FindClosestWindow(Degrees, &ClosestWindow, Wrap)) &&
           (ClosestWindow))
            MarkWindowContainer(ClosestWindow);
    }
}

internal void
KwmSpaceCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-fExperimental")
    {
        if(Tokens[2] == "previous")
            GoToPreviousSpace(false);
        else
            ActivateSpaceWithoutTransition(Tokens[2]);
    }
    else if(Tokens[1] == "-t")
    {
        if(Tokens[2] == "bsp")
            ResetWindowNodeTree(AXLibMainDisplay(), SpaceModeBSP);
        else if(Tokens[2] == "monocle")
            ResetWindowNodeTree(AXLibMainDisplay(), SpaceModeMonocle);
        else if(Tokens[2] == "float")
            ResetWindowNodeTree(AXLibMainDisplay(), SpaceModeFloating);
    }
    else if(Tokens[1] == "-r")
    {
        if(Tokens[2] == "focused")
        {
            ax_display *Display = AXLibMainDisplay();
            if(Display)
            {
                space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
                ApplyTreeNodeContainer(SpaceInfo->RootNode);
            }
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
        ax_display *Display = AXLibMainDisplay();
        if(Display)
        {
            SetNameOfActiveSpace(Display, Tokens[2]);
        }
    }
}

internal void
KwmDisplayCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-f")
    {
        if(Tokens[2] == "prev")
        {
            ax_display *Display = AXLibMainDisplay();
            if(Display)
                FocusDisplay(AXLibPreviousDisplay(Display));
        }
        else if(Tokens[2] == "next")
        {
            ax_display *Display = AXLibMainDisplay();
            if(Display)
                FocusDisplay(AXLibNextDisplay(Display));
        }
        else
        {
            int DisplayID = ConvertStringToInt(Tokens[2]);
            ax_display *Display = AXLibArrangementDisplay(DisplayID);
            if(Display)
                FocusDisplay(Display);
        }
    }
    else if(Tokens[1] == "-c")
    {
        if(Tokens[2] == "optimal")
            KWMSettings.SplitMode = SPLIT_OPTIMAL;
        else if(Tokens[2] == "vertical")
            KWMSettings.SplitMode = SPLIT_VERTICAL;
        else if(Tokens[2] == "horizontal")
            KWMSettings.SplitMode = SPLIT_HORIZONTAL;
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
            RotateBSPTree(ConvertStringToInt(Tokens[2]));
        }
    }
    else if(Tokens[1] == "save")
    {
        ax_display *Display = AXLibMainDisplay();
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        SaveBSPTreeToFile(Display, SpaceInfo, Tokens[2]);
    }
    else if(Tokens[1] == "restore")
    {
        LoadWindowNodeTree(AXLibMainDisplay(), Tokens[2]);
    }
}

internal void
KwmScratchpadCommand(std::vector<std::string> &Tokens, int ClientSockFD)
{
    if(Tokens[1] == "show")
    {
        ShowScratchpadWindow(ConvertStringToInt(Tokens[2]));
    }
    else if(Tokens[1] == "toggle")
    {
        ToggleScratchpadWindow(ConvertStringToInt(Tokens[2]));
    }
    else if(Tokens[1] == "hide")
    {
        HideScratchpadWindow(ConvertStringToInt(Tokens[2]));
    }
    else if(Tokens[1] == "add")
    {
        ax_application *Application = AXLibGetFocusedApplication();
        if(Application && Application->Focus)
            AddWindowToScratchpad(Application->Focus);
    }
    else if(Tokens[1] == "remove")
    {
        ax_application *Application = AXLibGetFocusedApplication();
        if(Application && Application->Focus)
            RemoveWindowFromScratchpad(Application->Focus);
    }
    else if(Tokens[1] == "list")
    {
        KwmWriteToSocket(ClientSockFD, GetWindowsOnScratchpad());
    }
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
