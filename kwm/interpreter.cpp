#include "kwm.h"

extern kwm_screen KWMScreen;
extern kwm_toggles KWMToggles;
extern kwm_focus KWMFocus;
extern kwm_mode KWMMode;
extern kwm_tiling KWMTiling;

std::string CreateStringFromTokens(std::vector<std::string> Tokens, int StartIndex)
{
    std::string Text = "";
    for(std::size_t TokenIndex = StartIndex; TokenIndex < Tokens.size(); ++TokenIndex)
    {
        Text += Tokens[TokenIndex];
        if(TokenIndex < Tokens.size() - 1)
            Text += " ";
    }

    return Text;
}

std::vector<std::string> SplitString(std::string Line, char Delim)
{
    std::vector<std::string> Elements;
    std::stringstream Stream(Line);
    std::string Temp;

    while(std::getline(Stream, Temp, Delim))
        Elements.push_back(Temp);

    return Elements;
}

// Command types
void KwmConfigCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "reload")
    {
        KwmReloadConfig();
    }
    else if(Tokens[1] == "prefix")
    {
        KwmSetGlobalPrefix(Tokens[2]);
    }
    else if(Tokens[1] == "prefix-timeout")
    {
        double Value = 0;
        std::stringstream Stream(Tokens[2]);
        Stream >> Value;
        KwmSetGlobalPrefixTimeout(Value);
    }
    else  if(Tokens[1] == "launchd")
    {
        if(Tokens[2] == "disable")
            RemoveKwmFromLaunchd();
        else if(Tokens[2] == "enable")
            AddKwmToLaunchd();
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
        if(Tokens[2] == "disable")
            KWMToggles.EnableTilingMode = false;
        else if(Tokens[2] == "enable")
            KWMToggles.EnableTilingMode = true;
    }
    else if(Tokens[1] == "capture")
    {
        int Index = 0;
        std::stringstream Stream(Tokens[2]);
        Stream >> Index;
        CaptureApplicationToScreen(Index, CreateStringFromTokens(Tokens, 3));
    }
    else if(Tokens[1] == "space")
    {
        if(Tokens[2] == "bsp")
            KWMMode.Space = SpaceModeBSP;
        else if(Tokens[2] == "monocle")
            KWMMode.Space = SpaceModeMonocle;
        else if(Tokens[2] == "float")
            KWMMode.Space = SpaceModeFloating;
    }
    else if(Tokens[1] == "focus")
    {
        if(Tokens[2] == "mouse-follows")
        {
            if(Tokens[3] == "disable")
                KWMToggles.UseMouseFollowsFocus = false;
            else if(Tokens[3] == "enable")
                KWMToggles.UseMouseFollowsFocus = true;
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
        else if(Tokens[2] == "disabled")
            KWMMode.Focus = FocusModeDisabled;
    }
    else if(Tokens[1] == "cycle-focus")
    {
        if(Tokens[2] == "screen")
            KWMMode.Cycle = CycleModeScreen;
        else if(Tokens[2] == "all")
            KWMMode.Cycle = CycleModeAll;
        else if(Tokens[2] == "disabled")
            KWMMode.Cycle = CycleModeDisabled;;
    }
    else if(Tokens[1] == "hotkeys")
    {
        if(Tokens[2] == "disable")
            KWMToggles.UseBuiltinHotkeys = false;
        else if(Tokens[2] == "enable")
            KWMToggles.UseBuiltinHotkeys = true;
    }
    else if(Tokens[1] == "dragndrop")
    {
        if(Tokens[2] == "disable")
            KWMToggles.EnableDragAndDrop = false;
        else if(Tokens[2] == "enable")
            KWMToggles.EnableDragAndDrop = true;
    }
    else if(Tokens[1] == "menu-fix")
    {
        if(Tokens[2] == "disable")
            KWMToggles.UseContextMenuFix = false;
        else if(Tokens[2] == "enable")
            KWMToggles.UseContextMenuFix = true;
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
        if(Tokens[2] == "left" || Tokens[2] == "right" ||
                Tokens[2] == "top" || Tokens[2] == "bottom")
        {
            int Value = 0;
            std::stringstream Stream(Tokens[3]);
            Stream >> Value;
            SetDefaultPaddingOfDisplay(Tokens[2], Value);
        }
    }
    else if(Tokens[1] == "gap")
    {
        if(Tokens[2] == "vertical" || Tokens[2] == "horizontal")
        {
            int Value = 0;
            std::stringstream Stream(Tokens[3]);
            Stream >> Value;
            SetDefaultGapOfDisplay(Tokens[2], Value);
        }
    }
    else if(Tokens[1] == "split-ratio")
    {
        double Value = 0;
        std::stringstream Stream(Tokens[2]);
        Stream >> Value;
        ChangeSplitRatio(Value);
    }
    else if(Tokens[1] == "screen")
    {
        int ScreenIndex = 0;
        std::stringstream Stream(Tokens[2]);
        Stream >> ScreenIndex;
        SetSpaceModeOfDisplay(ScreenIndex, Tokens[3]);
    }
}

void KwmReadCommand(std::vector<std::string> &Tokens, int ClientSockFD)
{
    if(Tokens[1] == "focused")
    {
        std::string Output;
        GetTagForCurrentSpace(Output);

        if(KWMFocus.Window)
            Output += " " + KWMFocus.Window->Owner + " - " + KWMFocus.Window->Name;

        KwmWriteToSocket(ClientSockFD, Output);
    }
    if(Tokens[1] == "marked")
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
    else if(Tokens[1] == "split-ratio")
    {
        std::string Output = std::to_string(KWMScreen.SplitRatio);
        Output.erase(Output.find_last_not_of('0') + 1, std::string::npos);
        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "split-mode")
    {
        std::string Output;
        if(KWMScreen.SplitMode == -1)
            Output = "Optimal";
        else if(KWMScreen.SplitMode == 1)
            Output = "Vertical";
        else if(KWMScreen.SplitMode == 2)
            Output = "Horizontal";

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
            Output = "disabled";

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[1] == "mouse-follows")
    {
        std::string Output;
        if(KWMToggles.UseMouseFollowsFocus)
            Output = "enabled";
        else 
            Output = "disabled";

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
        else if(KWMMode.Cycle == CycleModeAll)
            Output = "all";
        else 
            Output = "disabled";

        KwmWriteToSocket(ClientSockFD, Output);
    }
}

void KwmWindowCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-t")
    {
        if(Tokens[2] == "fullscreen")
            ToggleFocusedWindowFullscreen();
        else if(Tokens[2] == "parent")
            ToggleFocusedWindowParentContainer();
        else if(Tokens[2] == "float")
            ToggleFocusedWindowFloating();
        else if(Tokens[2] == "mark")
            MarkWindowContainer();
    }
    else if(Tokens[1] == "-c")
    {
        if(Tokens[2] == "split")
        {
            space_info *Space = &KWMScreen.Current->Space[KWMScreen.Current->ActiveSpace];
            tree_node *Node = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
            ToggleNodeSplitMode(KWMScreen.Current, Node->Parent);
        }
        else if(Tokens[2] == "reduce" || Tokens[2] == "expand")
        {
            double Ratio = 0.1;
            std::stringstream Stream(Tokens[3]);
            Stream >> Ratio;

            if(Tokens[2] == "reduce")
                ModifyContainerSplitRatio(-Ratio);
            else if(Tokens[2] == "expand")
                ModifyContainerSplitRatio(Ratio);
        }
        else if(Tokens[2] == "refresh")
        {
            ResizeWindowToContainerSize();
        }
    }
    else if(Tokens[1] == "-f")
    {
        if(Tokens[2] == "prev")
            ShiftWindowFocus(-1);
        else if(Tokens[2] == "next")
            ShiftWindowFocus(1);
        else if(Tokens[2] == "curr")
            FocusWindowBelowCursor();
    }
    else if(Tokens[1] == "-s")
    {
        if(Tokens[2] == "prev")
            SwapFocusedWindowWithNearest(-1);
        else if(Tokens[2] == "next")
            SwapFocusedWindowWithNearest(1);
        else if(Tokens[2] == "mark")
            SwapFocusedWindowWithMarked();
    }
    else if(Tokens[1] == "-x")
    {
        int Marked = KWMScreen.MarkedWindow;
        if(KWMFocus.Window && Marked == KWMFocus.Window->WID)
            return;

        ToggleWindowFloating(Marked);
        usleep(250000);
        KWMScreen.MarkedWindow = -1;
        ToggleWindowFloating(Marked);
    }
}

void KwmTreeCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-r")
    {
        if(Tokens[2] == "90" || Tokens[2] == "270" || Tokens[2] == "180")
        {
            int Deg = 0;
            std::stringstream Stream(Tokens[2]);
            Stream >> Deg;

            space_info *Space = &KWMScreen.Current->Space[KWMScreen.Current->ActiveSpace];
            if(Space->Mode == SpaceModeBSP)
            {
                RotateTree(Space->RootNode, Deg);
                CreateNodeContainers(KWMScreen.Current, Space->RootNode, false);
                ApplyNodeContainer(Space->RootNode, Space->Mode);
            }
        }
    }
    else if(Tokens[1] == "-c")
    {
        if(Tokens[2] == "refresh")
        {
            space_info *Space = &KWMScreen.Current->Space[KWMScreen.Current->ActiveSpace];
            ApplyNodeContainer(Space->RootNode, Space->Mode);
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

void KwmScreenCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-f")
    {
        if(Tokens[2] == "prev")
            GiveFocusToScreen(GetIndexOfPrevScreen(), NULL, false);
        else if(Tokens[2] == "next")
            GiveFocusToScreen(GetIndexOfNextScreen(), NULL, false);
        else 
        {
            int Index = 0;
            std::stringstream Stream(Tokens[2]);
            Stream >> Index;
            GiveFocusToScreen(Index, NULL, false);
        }
    }
    else if(Tokens[1] == "-s")
    {
        if(Tokens[2] == "optimal")
            KWMScreen.SplitMode = -1;
        else if(Tokens[2] == "vertical")
            KWMScreen.SplitMode = 1;
        else if(Tokens[2] == "horizontal")
            KWMScreen.SplitMode = 2;
    }
    else if(Tokens[1] == "-m")
    {
        if(IsApplicationCapturedByScreen(KWMFocus.Window))
            return;

        if(Tokens[2] == "prev")
            MoveWindowToDisplay(KWMFocus.Window, -1, true);
        else if(Tokens[2] == "next")
            MoveWindowToDisplay(KWMFocus.Window, 1, true);
        else
        {
            int Index = 0;
            std::stringstream Stream(Tokens[2]);
            Stream >> Index;
            MoveWindowToDisplay(KWMFocus.Window, Index, false);
        }
    }
}

void KwmSpaceCommand(std::vector<std::string> &Tokens)
{
    if(Tokens[1] == "-t")
    {
        if(Tokens[2] == "toggle")
            ToggleFocusedSpaceFloating();
        else if(Tokens[2] == "float")
            FloatFocusedSpace();
        else if(Tokens[2] == "bsp")
            TileFocusedSpace(SpaceModeBSP);
        else if(Tokens[2] == "monocle")
            TileFocusedSpace(SpaceModeMonocle);
    }
    else if(Tokens[1] == "-r")
    {
    }
    else if(Tokens[1] == "-p")
    {
        if(Tokens[3] == "left" || Tokens[3] == "right" ||
                Tokens[3] == "top" || Tokens[3] == "bottom")
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
        if(Tokens[3] == "vertical" || Tokens[3] == "horizontal")
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

void KwmBindCommand(std::vector<std::string> &Tokens)
{
    if(Tokens.size() > 2)
        KwmAddHotkey(Tokens[1], CreateStringFromTokens(Tokens, 2));
    else
        KwmAddHotkey(Tokens[1], "");
}
// ------------------------------------------------------------------------------------

void KwmInterpretCommand(std::string Message, int ClientSockFD)
{
    std::vector<std::string> Tokens = SplitString(Message, ' ');

    if(Tokens[0] == "quit")
        KwmQuit();
    else if(Tokens[0] == "config")
        KwmConfigCommand(Tokens);
    else if(Tokens[0] == "read")
        KwmReadCommand(Tokens, ClientSockFD);
    else if(Tokens[0] == "window")
        KwmWindowCommand(Tokens);
    else if(Tokens[0] == "screen")
        KwmScreenCommand(Tokens);
    else if(Tokens[0] == "space")
        KwmSpaceCommand(Tokens);
    else if(Tokens[0] == "tree")
        KwmTreeCommand(Tokens);
    else if(Tokens[0] == "write")
        KwmEmitKeystrokes(CreateStringFromTokens(Tokens, 1));
    else if(Tokens[0] == "bind")
        KwmBindCommand(Tokens);
    else if(Tokens[0] == "unbind")
        KwmRemoveHotkey(Tokens[1]);
}
