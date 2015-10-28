#include "kwm.h"

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

extern "C" KWM_HOTKEY_COMMANDS(KWMHotkeyCommands)
{
    bool Result = true;
    if(CmdKey && AltKey && CtrlKey)
    {
        switch(Keycode)
        {
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
            case kVK_ANSI_Q:
            {
                EX->KwmRestart();
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
                EX->KwmFocusMode = FocusAutoraise;
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
            case kVK_ANSI_R:
            {
                SysCommand = "sh " + EX->KwmFilePath + "/sobuild.sh";
            } break;
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
                SysCommand = "ytc seekfw";
            } break;
            // Window Resize
            case kVK_ANSI_H:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X, EX->FocusedWindow->Y,
                        EX->FocusedWindow->Width - 10, EX->FocusedWindow->Height);
            } break;
            case kVK_ANSI_J:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X, EX->FocusedWindow->Y,
                        EX->FocusedWindow->Width, EX->FocusedWindow->Height + 10);
            } break;
            case kVK_ANSI_K:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X, EX->FocusedWindow->Y,
                        EX->FocusedWindow->Width, EX->FocusedWindow->Height - 10);
            } break;
            case kVK_ANSI_L:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X, EX->FocusedWindow->Y,
                        EX->FocusedWindow->Width + 10, EX->FocusedWindow->Height);
            } break;
            case kVK_ANSI_F:
            {
                EX->ToggleFocusedWindowFullscreen();
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
    else if(CmdKey && CtrlKey && !AltKey)
    {
        switch(Keycode)
        {
            // Multiple Screens
            case kVK_ANSI_P:
            {
                EX->CycleFocusedWindowDisplay(-1);
            } break;
            case kVK_ANSI_N:
            {
                EX->CycleFocusedWindowDisplay(1);
            } break;
            // Move Window
            case kVK_ANSI_H:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X - 10, EX->FocusedWindow->Y,
                        EX->FocusedWindow->Width, EX->FocusedWindow->Height);
            } break;
            case kVK_ANSI_J:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X, EX->FocusedWindow->Y + 10,
                        EX->FocusedWindow->Width, EX->FocusedWindow->Height);
            } break;
            case kVK_ANSI_K:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X, EX->FocusedWindow->Y - 10,
                        EX->FocusedWindow->Width, EX->FocusedWindow->Height);
            } break;
            case kVK_ANSI_L:
            {
                EX->SetWindowDimensions(EX->FocusedWindowRef, EX->FocusedWindow,
                        EX->FocusedWindow->X + 10, EX->FocusedWindow->Y,
                        EX->FocusedWindow->Width, EX->FocusedWindow->Height);
            } break;
            default:
            {
                Result = false;
            };
        }
    }
    else if(CmdKey && AltKey && !CtrlKey)
    {
        switch(Keycode)
        {
            // Cycle focused window layout
            case kVK_ANSI_P:
            {
                EX->SwapFocusedWindowWithNearest(-1);
            } break;
            case kVK_ANSI_N:
            {
                EX->SwapFocusedWindowWithNearest(1);
            } break;
            // Focus a window
            case kVK_ANSI_H:
            {
                if(EX->FocusedWindow)
                {
                    EX->ShiftWindowFocus(-1);
                }
            } break;
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
