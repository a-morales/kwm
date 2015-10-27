#include "kwm.h"

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

extern "C" KWM_HOTKEY_COMMANDS(KWMHotkeyCommands)
{
    if(CmdKey && AltKey && CtrlKey)
    {
        if(Keycode == kVK_ANSI_T)
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

            return true;
        }
        else if(Keycode == kVK_ANSI_Q)
        {
            EX->KwmRestart();
        }
    }

    return false;
}

extern "C" KWM_HOTKEY_COMMANDS(SystemHotkeyCommands)
{
    if (CmdKey && !CtrlKey && !AltKey)
    {
        // Spotlight fix
        if (Keycode == kVK_Space)
        {
            EX->KwmFocusMode = FocusAutoraise;
            return true;
        }
    }

    return false;
}

extern "C" KWM_HOTKEY_COMMANDS(CustomHotkeyCommands)
{
    if(CmdKey && AltKey && CtrlKey)
    {
        // Start Applications
        std::string SysCommand = "";
        // New iterm2 Window
        if(Keycode == kVK_Return)
            SysCommand = "/Applications/iTerm.app/Contents/MacOS/iTerm2 --new-window &";
        // Rebuild hotkeys.cpp
        else if(Keycode == kVK_ANSI_R)
            SysCommand = "sh " + EX->KwmFilePath + "/sobuild.sh";
        // YTD - Media Player Controls
        else if(Keycode == kVK_ANSI_Z)
            SysCommand = "ytc prev";
        else if(Keycode == kVK_ANSI_X)
            SysCommand = "ytc play";
        else if(Keycode == kVK_ANSI_C)
            SysCommand = "ytc next";
        else if(Keycode == kVK_ANSI_V)
            SysCommand = "ytc stop";
        else if(Keycode == kVK_ANSI_A)
            SysCommand = "ytc volup";
        else if(Keycode == kVK_ANSI_D)
            SysCommand = "ytc voldown";
        else if(Keycode == kVK_ANSI_S)
            SysCommand = "ytc seekfw";

        if(SysCommand != "")
        {
            system(SysCommand.c_str());
            return true;
        }

        // Window Resize
        if(Keycode == kVK_ANSI_H)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X,
                    EX->FocusedWindow->Y,
                    EX->FocusedWindow->Width - 10,
                    EX->FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_J)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X,
                    EX->FocusedWindow->Y,
                    EX->FocusedWindow->Width,
                    EX->FocusedWindow->Height + 10);
        else if(Keycode == kVK_ANSI_K)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X,
                    EX->FocusedWindow->Y,
                    EX->FocusedWindow->Width,
                    EX->FocusedWindow->Height - 10);
        else if(Keycode == kVK_ANSI_L)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X,
                    EX->FocusedWindow->Y,
                    EX->FocusedWindow->Width + 10,
                    EX->FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_F)
        {
            EX->ToggleFocusedWindowFullscreen();
            return true;
        }
    }

    if(CmdKey && CtrlKey && !AltKey)
    {
        // Multiple Screens
        if(Keycode == kVK_ANSI_P || Keycode == kVK_ANSI_N)
        {
            if(Keycode == kVK_ANSI_P)
                EX->CycleFocusedWindowDisplay(-1);

            if(Keycode == kVK_ANSI_N)
                EX->CycleFocusedWindowDisplay(1);

            return true;
        }

        // Move Window
        if(Keycode == kVK_ANSI_H)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X - 10,
                    EX->FocusedWindow->Y,
                    EX->FocusedWindow->Width,
                    EX->FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_J)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X,
                    EX->FocusedWindow->Y + 10,
                    EX->FocusedWindow->Width,
                    EX->FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_K)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X,
                    EX->FocusedWindow->Y - 10,
                    EX->FocusedWindow->Width,
                    EX->FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_L)
            EX->SetWindowDimensions(EX->FocusedWindowRef,
                    EX->FocusedWindow,
                    EX->FocusedWindow->X + 10,
                    EX->FocusedWindow->Y,
                    EX->FocusedWindow->Width,
                    EX->FocusedWindow->Height);
    }

    if(CmdKey && AltKey && !CtrlKey)
    {
        // Cycle focused window layout
        if(Keycode == kVK_ANSI_P || Keycode == kVK_ANSI_N)
        {
            if(Keycode == kVK_ANSI_P)
                EX->SwapFocusedWindowWithNearest(-1);
            if(Keycode == kVK_ANSI_N)
                EX->SwapFocusedWindowWithNearest(1);

            return true;
        }

        // Focus a window
        if(Keycode == kVK_ANSI_H || Keycode == kVK_ANSI_L)
        {
            if(Keycode == kVK_ANSI_H && EX->FocusedWindow)
                EX->ShiftWindowFocus(-1);
            else if(Keycode == kVK_ANSI_L && EX->FocusedWindow)
                EX->ShiftWindowFocus(1);
        
            return true;
        }
    }
    
    if(CmdKey && !AltKey && !CtrlKey)
    {
        // disable retarded hotkey for minimizing an application
        if(Keycode == kVK_ANSI_M)
            return true;
        // disable retarded hotkey for hiding an application
        else if(Keycode == kVK_ANSI_H)
            return true;
        // disable cmd+tab window-swap
        else if(Keycode == kVK_Tab)
            return true;
    }

    return false;
}
