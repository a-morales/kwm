#include "kwm.h"

extern AXUIElementRef FocusedWindowRef;
extern window_info *FocusedWindow;
extern focus_option KwmFocusMode;

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

bool KwmHotkeyCommands(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode)
{
    if(CmdKey && AltKey && CtrlKey)
    {
        if(Keycode == kVK_ANSI_T)
        {
            if(KwmFocusMode == FocusFollowsMouse)
            {
                KwmFocusMode = FocusAutoraise;
                DEBUG("KwmFocusMode: Autoraise")
            }
            else if(KwmFocusMode == FocusAutoraise)
            {
                KwmFocusMode = FocusDisabled;
                DEBUG("KwmFocusMode: Disabled")
            }
            else if(KwmFocusMode == FocusDisabled)
            {
                KwmFocusMode = FocusFollowsMouse;
                DEBUG("KwmFocusMode: Focus-follows-mouse")
            }

            return true;
        }
    }

    return false;
}

bool SystemHotkeyPassthrough(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode)
{
    if (CmdKey && !CtrlKey && !AltKey)
    {
        // Spotlight fix
        if (Keycode == kVK_Space)
        {
            KwmFocusMode = FocusAutoraise;
            return true;
        }
        else if(Keycode == kVK_Tab)
            return true;
    }

    return false;
}

bool CustomHotkeyCommands(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode)
{
    if(CmdKey && AltKey && CtrlKey)
    {
        // Start Applications
        std::string SysCommand = "";
        // New iterm2 Window
        if(Keycode == kVK_Return)
            SysCommand = "/Applications/iTerm.app/Contents/MacOS/iTerm2 --new-window &";
        // YTD - Media Player Controls
        else if(Keycode == kVK_ANSI_Z)
            SysCommand = "ytc prev";
        else if(Keycode == kVK_ANSI_X)
            SysCommand = "ytc play";
        else if(Keycode == kVK_ANSI_C)
            SysCommand = "ytc next";
        else if(Keycode == kVK_ANSI_V)
            SysCommand = "ytc stop";

        if(SysCommand != "")
        {
            system(SysCommand.c_str());
            return true;
        }

        // Window Resize
        if(Keycode == kVK_ANSI_H)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X, 
                    FocusedWindow->Y, 
                    FocusedWindow->Width - 10, 
                    FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X, 
                    FocusedWindow->Y, 
                    FocusedWindow->Width, 
                    FocusedWindow->Height + 10);
        else if(Keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X, 
                    FocusedWindow->Y, 
                    FocusedWindow->Width, 
                    FocusedWindow->Height - 10);
        else if(Keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X, 
                    FocusedWindow->Y, 
                    FocusedWindow->Width + 10, 
                    FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_F)
            ToggleFocusedWindowFullscreen();
    }

    if(CmdKey && CtrlKey && !AltKey)
    {
        // Multiple Screens
        if(Keycode == kVK_ANSI_P || Keycode == kVK_ANSI_N)
        {
            if(Keycode == kVK_ANSI_P)
                CycleFocusedWindowDisplay(-1);

            if(Keycode == kVK_ANSI_N)
                CycleFocusedWindowDisplay(1);

            return true;
        }

        // Move Window
        if(Keycode == kVK_ANSI_H)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X - 10, 
                    FocusedWindow->Y, 
                    FocusedWindow->Width, 
                    FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X, 
                    FocusedWindow->Y + 10, 
                    FocusedWindow->Width, 
                    FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X, 
                    FocusedWindow->Y - 10, 
                    FocusedWindow->Width, 
                    FocusedWindow->Height);
        else if(Keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    FocusedWindow, 
                    FocusedWindow->X + 10, 
                    FocusedWindow->Y, 
                    FocusedWindow->Width, 
                    FocusedWindow->Height);
    }

    if(CmdKey && AltKey && !CtrlKey)
    {
        // Cycle focused window layout
        if(Keycode == kVK_ANSI_P || Keycode == kVK_ANSI_N)
        {
            if(Keycode == kVK_ANSI_P)
                SwapFocusedWindowWithNearest(-1);
            if(Keycode == kVK_ANSI_N)
                SwapFocusedWindowWithNearest(1);

            return true;
        }

        // Focus a window
        if(Keycode == kVK_ANSI_H || Keycode == kVK_ANSI_L)
        {
            if(Keycode == kVK_ANSI_H && FocusedWindow)
                ShiftWindowFocus(-1);
            else if(Keycode == kVK_ANSI_L && FocusedWindow)
                ShiftWindowFocus(1);
        
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
    }

    return false;
}
