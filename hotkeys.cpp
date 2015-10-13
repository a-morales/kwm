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
            ApplyLayoutForDisplay(GetDisplayOfWindow(&FocusedWindow)->id);
            return true;
        }

        // Window Layout
        window_layout Layout;
        Layout.name = "invalid";
        if(Keycode == kVK_ANSI_M)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "fullscreen");
        else if(Keycode == kVK_LeftArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "left vertical split");
        else if(Keycode == kVK_RightArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "right vertical split");
        else if(Keycode == kVK_UpArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "upper horizontal split");
        else if(Keycode == kVK_DownArrow)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "lower horizontal split");
        else if(Keycode == kVK_ANSI_P)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "upper left split");
        else if(Keycode == kVK_SPECIAL_Ø)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "lower left split");
        else if(Keycode == kVK_SPECIAL_Å)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "upper right split");
        else if(Keycode == kVK_SPECIAL_Æ)
            Layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "lower right split");

        if(Layout.name != "invalid")
        {
            SetWindowDimensions(FocusedWindowRef, &FocusedWindow, Layout.x, Layout.y, Layout.width, Layout.height);
            return true;
        }

        // Window Resize
        if(Keycode == kVK_ANSI_H)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width - 10, 
                    FocusedWindow.height);
        else if(Keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height + 10);
        else if(Keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height - 10);
        else if(Keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width + 10, 
                    FocusedWindow.height);
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
                    FocusedWindow.x - 10, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
        else if(Keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y + 10, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
        else if(Keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y - 10, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
        else if(Keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x + 10, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
    }

    if(CmdKey && AltKey && !CtrlKey)
    {
        // Cycle focused window layout
        if(Keycode == kVK_ANSI_P || Keycode == kVK_ANSI_N)
        {
            if(Keycode == kVK_ANSI_P)
                CycleFocusedWindowLayout(GetDisplayOfWindow(&FocusedWindow)->id, -1);

            if(Keycode == kVK_ANSI_N)
                CycleFocusedWindowLayout(GetDisplayOfWindow(&FocusedWindow)->id, 1);

            return true;
        }

        // Cycle window inside focused layout
        if(Keycode == kVK_Tab)
        {
            CycleWindowInsideLayout(GetDisplayOfWindow(&FocusedWindow)->id);
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
