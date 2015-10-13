#include "kwm.h"

extern AXUIElementRef FocusedWindowRef;
extern window_info FocusedWindow;
extern bool ToggleTap;
extern bool EnableAutoraise;

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

bool KwmHotkeyCommands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
{
    if(cmd_key && alt_key && ctrl_key)
    {
        if(keycode == kVK_ANSI_T)
        {
            ToggleTap = !ToggleTap;
            std::cout << (ToggleTap ? "tap enabled" : "tap disabled") << std::endl;
            return true;
        }

        if(keycode == kVK_ANSI_R)
        {
            EnableAutoraise = !EnableAutoraise;
            std::cout << (EnableAutoraise ? "autoraise enabled" : "autoraise disabled") << std::endl;
            return true;
        }
    }

    return false;
}

bool SystemHotkeyPassthrough(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
{
    // Spotlight fix
    if (cmd_key && !ctrl_key && !alt_key)
    {
        if (keycode == kVK_Space)
        {
            ToggleTap = false;
            std::cout << "tap disabled" << std::endl;
            return true;
        }
        else if(keycode == kVK_Tab)
            return true;
    }

    return false;
}

bool CustomHotkeyCommands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
{
    if(cmd_key && alt_key && ctrl_key)
    {
        // Start Applications
        std::string sys_command = "";
        // New iterm2 Window
        if(keycode == kVK_ANSI_1)
            sys_command = "/Applications/iTerm.app/Contents/MacOS/iTerm2 --new-window &";
        // YTD - Media Player Controls
        else if(keycode == kVK_ANSI_Z)
            sys_command = "ytc prev";
        else if(keycode == kVK_ANSI_X)
            sys_command = "ytc play";
        else if(keycode == kVK_ANSI_C)
            sys_command = "ytc next";
        else if(keycode == kVK_ANSI_V)
            sys_command = "ytc stop";

        if(sys_command != "")
        {
            system(sys_command.c_str());
            return true;
        }

        // Toggle Screen Layout
        if(keycode == kVK_Space)
        {
            ApplyLayoutForDisplay(GetDisplayOfWindow(&FocusedWindow)->id);
            return true;
        }

        // Window Layout
        window_layout layout;
        layout.name = "invalid";
        if(keycode == kVK_ANSI_M)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "fullscreen");
        else if(keycode == kVK_LeftArrow)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "left vertical split");
        else if(keycode == kVK_RightArrow)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "right vertical split");
        else if(keycode == kVK_UpArrow)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "upper horizontal split");
        else if(keycode == kVK_DownArrow)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "lower horizontal split");
        else if(keycode == kVK_ANSI_P)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "upper left split");
        else if(keycode == kVK_SPECIAL_Ø)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "lower left split");
        else if(keycode == kVK_SPECIAL_Å)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "upper right split");
        else if(keycode == kVK_SPECIAL_Æ)
            layout = GetWindowLayoutForScreen(GetDisplayOfWindow(&FocusedWindow)->id, "lower right split");

        if(layout.name != "invalid")
        {
            SetWindowDimensions(FocusedWindowRef, &FocusedWindow, layout.x, layout.y, layout.width, layout.height);
            return true;
        }

        // Window Resize
        if(keycode == kVK_ANSI_H)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width - 10, 
                    FocusedWindow.height);
        else if(keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height + 10);
        else if(keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height - 10);
        else if(keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y, 
                    FocusedWindow.width + 10, 
                    FocusedWindow.height);
    }

    if(cmd_key && ctrl_key && !alt_key)
    {
        // Multiple Screens
        if(keycode == kVK_ANSI_P || keycode == kVK_ANSI_N)
        {
            if(keycode == kVK_ANSI_P)
                CycleFocusedWindowDisplay(-1);

            if(keycode == kVK_ANSI_N)
                CycleFocusedWindowDisplay(1);

            return true;
        }

        // Move Window
        if(keycode == kVK_ANSI_H)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x - 10, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
        else if(keycode == kVK_ANSI_J)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y + 10, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
        else if(keycode == kVK_ANSI_K)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x, 
                    FocusedWindow.y - 10, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
        else if(keycode == kVK_ANSI_L)
            SetWindowDimensions(FocusedWindowRef, 
                    &FocusedWindow, 
                    FocusedWindow.x + 10, 
                    FocusedWindow.y, 
                    FocusedWindow.width, 
                    FocusedWindow.height);
    }

    if(cmd_key && alt_key && !ctrl_key)
    {
        // Cycle focused window layout
        if(keycode == kVK_ANSI_P || keycode == kVK_ANSI_N)
        {
            if(keycode == kVK_ANSI_P)
                CycleFocusedWindowLayout(GetDisplayOfWindow(&FocusedWindow)->id, -1);

            if(keycode == kVK_ANSI_N)
                CycleFocusedWindowLayout(GetDisplayOfWindow(&FocusedWindow)->id, 1);

            return true;
        }

        // Cycle window inside focused layout
        if(keycode == kVK_Tab)
        {
            CycleWindowInsideLayout(GetDisplayOfWindow(&FocusedWindow)->id);
            return true;
        }

        // Focus a window
        if(keycode == kVK_ANSI_H || keycode == kVK_ANSI_L)
        {
            if(keycode == kVK_ANSI_H)
                ShiftWindowFocus("prev");
            else if(keycode == kVK_ANSI_L)
                ShiftWindowFocus("next");
        
            return true;
        }
    }
    
    if(cmd_key && !alt_key && !ctrl_key)
    {
        // disable retarded hotkey for minimizing an application
        if(keycode == kVK_ANSI_M)
            return true;
        // disable retarded hotkey for hiding an application
        else if(keycode == kVK_ANSI_H)
            return true;
    }

    return false;
}
