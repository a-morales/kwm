#include "kwm.h"

extern AXUIElementRef focused_window_ref;
extern window_info focused_window;
extern bool toggle_tap;
extern bool enable_auto_raise;

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

bool kwm_hotkey_commands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
{
    if(cmd_key && alt_key && ctrl_key)
    {
        if(keycode == kVK_ANSI_T)
        {
            toggle_tap = !toggle_tap;
            std::cout << (toggle_tap ? "tap enabled" : "tap disabled") << std::endl;
            return true;
        }

        if(keycode == kVK_ANSI_R)
        {
            enable_auto_raise = !enable_auto_raise;
            std::cout << (enable_auto_raise ? "autoraise enabled" : "autoraise disabled") << std::endl;
            return true;
        }
    }

    return false;
}

bool system_hotkey_passthrough(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
{
    // Spotlight fix
    if (cmd_key && !ctrl_key && !alt_key)
    {
        if (keycode == kVK_Space)
        {
            toggle_tap = false;
            std::cout << "tap disabled" << std::endl;
            return true;
        }
        else if(keycode == kVK_Tab)
            return true;
    }

    return false;
}

bool custom_hotkey_commands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
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
            apply_layout_for_display(get_display_of_window(&focused_window)->id);
            return true;
        }

        // Window Layout
        window_layout layout;
        layout.name = "invalid";
        if(keycode == kVK_ANSI_M)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "fullscreen");
        else if(keycode == kVK_LeftArrow)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "left vertical split");
        else if(keycode == kVK_RightArrow)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "right vertical split");
        else if(keycode == kVK_UpArrow)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "upper horizontal split");
        else if(keycode == kVK_DownArrow)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "lower horizontal split");
        else if(keycode == kVK_ANSI_P)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "upper left split");
        else if(keycode == kVK_SPECIAL_Ø)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "lower left split");
        else if(keycode == kVK_SPECIAL_Å)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "upper right split");
        else if(keycode == kVK_SPECIAL_Æ)
            layout = get_window_layout_for_screen(get_display_of_window(&focused_window)->id, "lower right split");

        if(layout.name != "invalid")
        {
            set_window_dimensions(focused_window_ref, &focused_window, layout.x, layout.y, layout.width, layout.height);
            return true;
        }

        // Window Resize
        if(keycode == kVK_ANSI_H)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x, 
                    focused_window.y, 
                    focused_window.width - 10, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_J)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height + 10);
        else if(keycode == kVK_ANSI_K)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height - 10);
        else if(keycode == kVK_ANSI_L)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x, 
                    focused_window.y, 
                    focused_window.width + 10, 
                    focused_window.height);
    }

    if(cmd_key && ctrl_key && !alt_key)
    {
        // Multiple Screens
        if(keycode == kVK_ANSI_P || keycode == kVK_ANSI_N)
        {
            if(keycode == kVK_ANSI_P)
                cycle_focused_window_display(-1);

            if(keycode == kVK_ANSI_N)
                cycle_focused_window_display(1);

            return true;
        }

        // Move Window
        if(keycode == kVK_ANSI_H)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x - 10, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_J)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x, 
                    focused_window.y + 10, 
                    focused_window.width, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_K)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x, 
                    focused_window.y - 10, 
                    focused_window.width, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_L)
            set_window_dimensions(focused_window_ref, 
                    &focused_window, 
                    focused_window.x + 10, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height);
    }

    if(cmd_key && alt_key && !ctrl_key)
    {
        // Cycle focused window layout
        if(keycode == kVK_ANSI_P || keycode == kVK_ANSI_N)
        {
            if(keycode == kVK_ANSI_P)
                cycle_focused_window_layout(get_display_of_window(&focused_window)->id, -1);

            if(keycode == kVK_ANSI_N)
                cycle_focused_window_layout(get_display_of_window(&focused_window)->id, 1);

            return true;
        }

        // Cycle window inside focused layout
        if(keycode == kVK_Tab)
        {
            cycle_window_inside_layout(get_display_of_window(&focused_window)->id);
            return true;
        }

        // Focus a window
        if(keycode == kVK_ANSI_H || keycode == kVK_ANSI_L)
        {
            if(keycode == kVK_ANSI_H)
                shift_window_focus("prev");
            else if(keycode == kVK_ANSI_L)
                shift_window_focus("next");
        
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
