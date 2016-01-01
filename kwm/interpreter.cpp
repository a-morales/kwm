#include "kwm.h"

extern screen_info *Screen;
extern std::vector<std::string> FloatingAppLst;
extern window_info *FocusedWindow;
extern focus_option KwmFocusMode;
extern int KwmSplitMode;
extern bool KwmEnableTilingMode;
extern bool KwmUseBuiltinHotkeys;
extern bool KwmEnableDragAndDrop;
extern bool KwmUseContextMenuFix;

std::vector<std::string> SplitString(std::string Line, char Delim)
{
    std::vector<std::string> Elements;
    std::stringstream Stream(Line);
    std::string Temp;

    while(std::getline(Stream, Temp, Delim))
        Elements.push_back(Temp);

    return Elements;
}

void KwmInterpretCommand(std::string Message, int ClientSockFD)
{
    std::vector<std::string> Tokens = SplitString(Message, ' ');

    if(Tokens[0] == "quit")
    {
        KwmQuit();
    }
    else if(Tokens[0] == "config")
    {
        if(Tokens[1] == "launchd")
        {
            if(Tokens[2] == "disable")
                RemoveKwmFromLaunchd();
            else if(Tokens[2] == "enable")
                AddKwmToLaunchd();
        }
        else if(Tokens[1] == "tiling")
        {
            if(Tokens[2] == "disable")
                KwmEnableTilingMode = false;
            else if(Tokens[2] == "enable")
                KwmEnableTilingMode = true;
        }
        else if(Tokens[1] == "focus")
        {
            if(Tokens[2] == "toggle")
            {
                if(KwmFocusMode == FocusModeDisabled)
                    KwmFocusMode = FocusModeAutofocus;
                else if(KwmFocusMode == FocusModeAutofocus)
                    KwmFocusMode = FocusModeAutoraise;
                else if(KwmFocusMode == FocusModeAutoraise)
                    KwmFocusMode = FocusModeDisabled;
            }
            else if(Tokens[2] == "autofocus")
                KwmFocusMode = FocusModeAutofocus;
            else if(Tokens[2] == "autoraise")
                KwmFocusMode = FocusModeAutoraise;
            else if(Tokens[2] == "disabled")
                KwmFocusMode = FocusModeDisabled;
        }
        else if(Tokens[1] == "hotkeys")
        {
            if(Tokens[2] == "disable")
                KwmUseBuiltinHotkeys = false;
            else if(Tokens[2] == "enable")
                KwmUseBuiltinHotkeys = true;
        }
        else if(Tokens[1] == "dragndrop")
        {
            if(Tokens[2] == "disable")
                KwmEnableDragAndDrop = false;
            else if(Tokens[2] == "enable")
                KwmEnableDragAndDrop = true;
        }
        else if(Tokens[1] == "menu-fix")
        {
            if(Tokens[2] == "disable")
                KwmUseContextMenuFix = false;
            else if(Tokens[2] == "enable")
                KwmUseContextMenuFix = true;
        }
        else if(Tokens[1] == "float")
        {
            std::string AppName;
            for(int TokenIndex = 2; TokenIndex < Tokens.size(); ++TokenIndex)
            {
                AppName += Tokens[TokenIndex];
                if(TokenIndex < Tokens.size() - 1)
                    AppName += " ";
            }
            FloatingAppLst.push_back(AppName);
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
    }
    else if(Tokens[0] == "focused")
    {
        if(FocusedWindow)
            KwmWriteToSocket(ClientSockFD, FocusedWindow->Owner + " - " + FocusedWindow->Name);
    }
    else if(Tokens[0] == "window")
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
                tree_node *Node = GetNodeFromWindowID(Screen->Space[Screen->ActiveSpace].RootNode, FocusedWindow->WID);
                ToggleNodeSplitMode(Screen, Node->Parent);
            }
            else if(Tokens[2] == "refresh")
                ResizeWindowToContainerSize();

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
    }
    else if(Tokens[0] == "screen")
    {
        if(Tokens[1] == "-s")
        {
            if(Tokens[2] == "optimal")
                KwmSplitMode = -1;
            else if(Tokens[2] == "vertical")
                KwmSplitMode = 1;
            else if(Tokens[2] == "horizontal")
                KwmSplitMode = 2;
        }
        else if(Tokens[1] == "-m")
        {
            if(Tokens[2] == "prev")
                CycleFocusedWindowDisplay(-1, true);
            else if(Tokens[2] == "next")
                CycleFocusedWindowDisplay(1, true);
            else
            {
                int Index = 0;
                std::stringstream Stream(Tokens[2]);
                Stream >> Index;
                CycleFocusedWindowDisplay(Index, false);
            }
        }
    }
    else if(Tokens[0] == "space")
    {
        if(Tokens[1] == "-t")
        {
            if(Tokens[2] == "toggle")
                ToggleFocusedSpaceFloating();
            else if(Tokens[2] == "float")
                FloatFocusedSpace();
            else if(Tokens[2] == "bsp")
                TileFocusedSpace(SpaceModeBSP);
        }
        else if(Tokens[1] == "-r")
        {
            if(Tokens[2] == "90" || Tokens[2] == "270" || Tokens[2] == "180")
            {
                int Deg = 0;
                std::stringstream Stream(Tokens[2]);
                Stream >> Deg;
                RotateTree(Screen->Space[Screen->ActiveSpace].RootNode, Deg);
                CreateNodeContainers(Screen, Screen->Space[Screen->ActiveSpace].RootNode, false);
                ApplyNodeContainer(Screen->Space[Screen->ActiveSpace].RootNode);
            }
        }
        else if(Tokens[1] == "-m")
        {
            if(Tokens[2] == "left")
                MoveContainerSplitter(1, -10);
            else if(Tokens[2] == "right")
                MoveContainerSplitter(1, 10);
            else if(Tokens[2] == "up")
                MoveContainerSplitter(2, -10);
            else if(Tokens[2] == "down")
                MoveContainerSplitter(2, 10);
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
    else if(Tokens[0] == "write")
    {
        std::string Text = "";
        for(int TokenIndex = 1; TokenIndex < Tokens.size(); ++TokenIndex)
        {
            Text += Tokens[TokenIndex];
            if(TokenIndex < Tokens.size() - 1)
              Text += " ";
        }

        KwmEmitKeystrokes(Text);
    }
}
