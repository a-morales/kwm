#include "kwm.h"

extern AXUIElementRef FocusedWindowRef;
extern window_info FocusedWindow;
extern bool ToggleTap;
extern bool EnableAutoraise;

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

bool KwmHotkeyCommands(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode)
{
    if(CmdKey && AltKey && CtrlKey)
    {
        if(Keycode == kVK_ANSI_T)
        {
            ToggleTap = !ToggleTap;
            std::cout << (ToggleTap ? "tap enabled" : "tap disabled") << std::endl;
            return true;
        }

        if(Keycode == kVK_ANSI_R)
        {
            EnableAutoraise = !EnableAutoraise;
            std::cout << (EnableAutoraise ? "autoraise enabled" : "autoraise disabled") << std::endl;
            return true;
        }
    }

    return false;
}

bool SystemHotkeyPassthrough(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode)
{
    // Spotlight fix
    if (CmdKey && !CtrlKey && !AltKey)
    {
        if (Keycode == kVK_Space)
        {
            ToggleTap = false;
            std::cout << "tap disabled" << std::endl;
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
        if(Keycode == kVK_ANSI_1)
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

        // Toggle Screen Layout
        if(Keycode == kVK_Space)
        {
            ApplyLayoutForDisplay(GetDisplayOfWindow(&FocusedWindow)->ID);
            return true;
        }

        // Window Layout
        window_layout Layout;
        Layout.Name = "invalid";
        if(Keycode == kVK_ANSI_M)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "fullscreen");
        else if(Keycode == kVK_LeftArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "left vertical split");
        else if(Keycode == kVK_RightArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "right vertical split");
        else if(Keycode == kVK_UpArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "upper horizontal split");
        else if(Keycode == kVK_DownArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "lower horizontal split");
        else if(Keycode == kVK_ANSI_P)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "upper left split");
        else if(Keycode == kVK_SPECIAL_Ø)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "lower left split");
        else if(Keycode == kVK_SPECIAL_Å)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "upper right split");
        else if(Keycode == kVK_SPECIAL_Æ)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->ID, "lower right split");

        if(Layout.Name != "invalid")
        {
            SetWindowDimensions(FocusedWindowRef, &FocusedWindow, Layout.X, Layout.Y, Layout.Width, Layout.Height);
            return true;
        }

        // Window Resize
        if(Keycode == kVK_ANSI_H)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.X, 
                    FocusedWindow.Y, 
                    FocusedWindow.Width - 10, 
                    FocusedWindow.Height);
        else if(Keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.X, 
                    FocusedWindow.Y, 
                    FocusedWindow.Width, 
                    FocusedWindow.Height + 10);
        else if(Keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.X, 
                    FocusedWindow.Y, 
                    FocusedWindow.Width, 
                    FocusedWindow.Height - 10);
        else if(Keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.X, 
                    FocusedWindow.Y, 
                    FocusedWindow.Width + 10, 
                    FocusedWindow.Height);
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
                    &FocusedWindow, 
                    FocusedWindow.X - 10, 
                    FocusedWindow.Y, 
                    FocusedWindow.Width, 
                    FocusedWindow.Height);
        else if(Keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.X, 
                    FocusedWindow.Y + 10, 
                    FocusedWindow.Width, 
                    FocusedWindow.Height);
        else if(Keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.X, 
                    FocusedWindow.Y - 10, 
                    FocusedWindow.Width, 
                    FocusedWindow.Height);
        else if(Keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.X + 10, 
                    FocusedWindow.Y, 
                    FocusedWindow.Width, 
                    FocusedWindow.Height);
    }

    if(CmdKey && AltKey && !CtrlKey)
    {
        // Cycle focused window layout
        if(Keycode == kVK_ANSI_P || Keycode == kVK_ANSI_N)
        {
            if(Keycode == kVK_ANSI_P)
                CycleFocusedWindowLayout(GetDisplayOfWindow(&FocusedWindow)->ID, -1);

            if(Keycode == kVK_ANSI_N)
                CycleFocusedWindowLayout(GetDisplayOfWindow(&FocusedWindow)->ID, 1);

            return true;
        }

        // Cycle window inside focused layout
        if(Keycode == kVK_Tab)
        {
            CycleWindowInsideLayout(GetDisplayOfWindow(&FocusedWindow)->ID);
            return true;
        }

        // Focus a window
        if(Keycode == kVK_ANSI_H || Keycode == kVK_ANSI_L)
        {
            if(Keycode == kVK_ANSI_H)
                ShiftWindowFocus("prev");
            else if(Keycode == kVK_ANSI_L)
                ShiftWindowFocus("next");
        
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
