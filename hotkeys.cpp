#include "kwm.h"

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

extern "C" KWM_KEY_REMAP(RemapKeys)
{
    *Result = -1;

    /*
    if(*CmdKey && *AltKey && *CtrlKey)
    {
    }

    switch(Keycode)
    {
        case kVK_ANSI_A:
        {
            *Result = kVK_ANSI_S;
        } break;
    }
    */
}

extern "C" KWM_HOTKEY_COMMANDS(KWMHotkeyCommands)
{
    bool Result = true;
    if(CmdKey && AltKey && CtrlKey)
    {
        switch(Keycode)
        {
            // Toggle Focus-Mode
            case kVK_ANSI_T:
            {
                if(EX->KwmFocusMode == FocusFollowsMouse)
                {
                    EX->KwmFocusMode = FocusAutoraise;
                    DEBUG("KwmFocusMode: Autoraise")
                }
                else if(EX->KwmFocusMode == FocusAutoraise)
                {
                    EX->KwmFocusMode = FocusDisabled;
                    DEBUG("KwmFocusMode: Disabled")
                }
                else if(EX->KwmFocusMode == FocusDisabled)
                {
                    EX->KwmFocusMode = FocusFollowsMouse;
                    DEBUG("KwmFocusMode: Focus-follows-mouse")
                }
            } break;
            // Mark Container to split
            case kVK_ANSI_M:
            {
                EX->MarkWindowContainer();
            } break;
            // Toggle window floating
            case kVK_ANSI_W:
            {
                bool Found = false;
                for(int Index = 0; Index < EX->FloatingWindowLst.size(); ++Index)
                {
                    if(EX->FloatingWindowLst[Index] == EX->FocusedWindow->WID)
                    {
                        EX->FloatingWindowLst.erase(EX->FloatingWindowLst.begin() + Index);
                        Found = true;
                        EX->KwmFocusMode = FocusAutoraise;
                        EX->AddWindowToTree();
                        break;
                    }
                }

                if(!Found)
                {
                    EX->FloatingWindowLst.push_back(EX->FocusedWindow->WID);
                    EX->KwmFocusMode = FocusFollowsMouse;
                    EX->RemoveWindowFromTree();
                }
            } break;
            // Reapply node container to window 
            case kVK_ANSI_R:
            {
                EX->ResizeWindowToContainerSize();
            } break;
            // Restart Kwm
            case kVK_ANSI_Q:
            {
                EX->KwmRestart();
            } break;
            // Use Width/Height ratio to determine split mode
            case kVK_ANSI_O:
            {
                EX->KwmSplitMode = -1;
            } break;
            // Vertical Split-Mode
            case kVK_ANSI_7:
            {
                EX->KwmSplitMode = 1;
            } break;
            // Horizontal Split-Mode
            case kVK_ANSI_Slash:
            {
                EX->KwmSplitMode = 2;
            } break;
            // Resize Panes
            case kVK_ANSI_H:
            {
                EX->MoveContainerSplitter(1, -10);
            } break;
            case kVK_ANSI_L:
            {
                EX->MoveContainerSplitter(1, 10);
            } break;
            case kVK_ANSI_J:
            {
                EX->MoveContainerSplitter(2, 10);
            } break;
            case kVK_ANSI_K:
            {
                EX->MoveContainerSplitter(2, -10);
            } break;
            // Toggle Fullscreen / Parent Container
            case kVK_ANSI_F:
            {
                EX->ToggleFocusedWindowFullscreen();
            } break;
            case kVK_ANSI_P:
            {
                EX->ToggleFocusedWindowParentContainer();
            } break;
            default:
            {
                Result = false;
            } break;
        }
    }
    else if(CmdKey && AltKey && !CtrlKey)
    {
        switch(Keycode)
        {
            // Swap focused window with the previous window
            case kVK_ANSI_P:
            {
                EX->SwapFocusedWindowWithNearest(-1);
            } break;
            // Swap focused window with the next window
            case kVK_ANSI_N:
            {
                EX->SwapFocusedWindowWithNearest(1);
            } break;
            // Shift focus to the previous window
            case kVK_ANSI_H:
            {
                if(EX->FocusedWindow)
                {
                    EX->ShiftWindowFocus(-1);
                }
            } break;
            // Shift focus to the next window
            case kVK_ANSI_L:
            {
                if(EX->FocusedWindow)
                {
                    EX->ShiftWindowFocus(1);
                }
            } break;
            default:
            {
                Result = false;
            } break;
        }
    }
    else if(CmdKey && CtrlKey && !AltKey)
    {
        switch(Keycode)
        {
            // Send window to previous screen
            case kVK_ANSI_P:
            {
                EX->CycleFocusedWindowDisplay(-1);
            } break;
            // Send window to next screen
            case kVK_ANSI_N:
            {
                EX->CycleFocusedWindowDisplay(1);
            } break;
            /*
            case kVK_ANSI_H:
            {
                //ReflectTreeVertically
            } break;
            case kVK_ANSI_L:
            {
                //ReflectTreeVertically
            } break;
            */
            default:
            {
                Result = false;
            };
        }
    }
    else
    {
        Result = false;
    }

    return Result;
}

extern "C" KWM_HOTKEY_COMMANDS(SystemHotkeyCommands)
{
    bool Result = true;
    if (CmdKey && !CtrlKey && !AltKey)
    {
        switch(Keycode)
        {
            // Spotlight fix
            case kVK_Space:
            {
                //EX->KwmFocusMode = FocusAutoraise;
            } break;
            default:
            {
                Result = false;
            } break;
        }
    }
    else
    {
        Result = false;
    }

    return Result;
}

extern "C" KWM_HOTKEY_COMMANDS(CustomHotkeyCommands)
{
    bool Result = true;
    if(CmdKey && AltKey && CtrlKey)
    {
        // Launch Application
        std::string SysCommand = "";
        switch(Keycode)
        {
            // Open New iTerm Window
            case kVK_Return:
            {
                SysCommand = "/Applications/iTerm.app/Contents/MacOS/iTerm2 --new-window &";
            } break;
            // Rebuild Hotkeys
            // case kVK_ANSI_R:
            // {
            //     SysCommand = "sh " + EX->KwmFilePath + "/sobuild.sh";
            // } break;
            // YTD - Media Player Controls
            case kVK_ANSI_Z:
            {
                SysCommand = "ytc prev";
            } break;
            case kVK_ANSI_X:
            {
                SysCommand = "ytc play";
            } break;
            case kVK_ANSI_C:
            {
                SysCommand = "ytc next";
            } break;
            case kVK_ANSI_V:
            {
                SysCommand = "ytc stop";
            } break;
            case kVK_ANSI_A:
            {
                SysCommand = "ytc volup";
            } break;
            case kVK_ANSI_D:
            {
                SysCommand = "ytc voldown";
            } break;
            case kVK_ANSI_S:
            {
                SysCommand = "ytc mkfav";
            } break;
            case kVK_LeftArrow:
            {
                SysCommand = "ytc seekbk";
            } break;
            case kVK_RightArrow:
            {
                SysCommand = "ytc seekfw";
            } break;
            case kVK_ANSI_Grave:
            {
                SysCommand = "ytc fav";
            } break;
            default:
            {
                Result = false;
            } break;
        }

        if(SysCommand != "")
        {
            system(SysCommand.c_str());
        }
    }
    else if(CmdKey && !AltKey && !CtrlKey)
    {
        switch(Keycode)
        {
            // disable retarded hotkey for minimizing an application
            case kVK_ANSI_M:
            {
            } break;
            // disable retarded hotkey for hiding an application
            case kVK_ANSI_H:
            {
            } break;
            // disable cmd+tab window-swap
            case kVK_Tab:
            {
            } break;
            default:
            {
                Result = false;
            } break;
        }
    }
    else
    {
        Result = false;
    }

    return Result;
}
