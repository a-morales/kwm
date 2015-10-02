#include "ffmouse.h"

static CGWindowListOption osx_window_list_option = kCGWindowListOptionOnScreenOnly | 
                                                   kCGWindowListExcludeDesktopElements;

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

static std::vector<screen_info> display_lst;
static std::vector<window_info> window_lst;
static std::vector<screen_layout> screen_layout_lst;
static std::vector<window_layout> layout_lst;

static ProcessSerialNumber focused_psn;
static AXUIElementRef focused_window_ref;
static window_info focused_window;

static bool toggle_tap = true;
static bool enable_auto_raise = true;

static uint32_t max_display_count = 5;
static uint32_t active_displays_count;
static CGDirectDisplayID active_displays[5];

bool check_privileges()
{
    if (AXAPIEnabled())
        return true;

    if(AXIsProcessTrusted())
        return true;

    std::cout << "not trusted" << std::endl;
    return false;
}

void request_privileges()
{
    if(AXMakeProcessTrusted(CFSTR("/usr/local/bin/ffmouse")) != kAXErrorSuccess)
        fatal("Could not make trusted!");

    std::cout << "is now trusted.." << std::endl;
}

void get_active_displays()
{
    CGGetActiveDisplayList(max_display_count, (CGDirectDisplayID*)&active_displays, &active_displays_count);
    for(int display_index = 0; display_index < active_displays_count; ++display_index)
    {
        CGRect display_rect = CGDisplayBounds(active_displays[display_index]);
        screen_info screen;
        screen.id = display_index;
        screen.x = display_rect.origin.x;
        screen.y = display_rect.origin.y;
        screen.width = display_rect.size.width;
        screen.height = display_rect.size.height;
        display_lst.push_back(screen);
    }
}

void cycle_focused_window_screen(int shift)
{
    screen_info *cur_screen = get_display_of_window(&focused_window);
    int new_screen_index;

    if(shift == 1)
        new_screen_index = (cur_screen->id + 1 >= active_displays_count) ? 0 : cur_screen->id + 1;
    else if(shift == -1)
        new_screen_index = (cur_screen->id - 1 < 0) ? active_displays_count - 1 : cur_screen->id - 1;
    else
        return;

    screen_info *screen = &display_lst[new_screen_index];
    set_window_dimensions(focused_window_ref,
            &focused_window,
            screen->x + 10, 
            screen->y + 10,
            focused_window.width,
            focused_window.height);
}

screen_info *get_display_of_window(window_info *window)
{
    for(int display_index = 0; display_index < active_displays_count; ++display_index)
    {
        screen_info *screen = &display_lst[display_index];
        if(window->x >= screen->x && window->x <= screen->x + screen->width)
            return screen;
    }

    return NULL;
}

void set_window_layout_values(window_layout *layout, int x, int y, int width, int height)
{
    layout->x = x;
    layout->y = y;
    layout->width = width;
    layout->height = height;
}

std::vector<window_info*> get_all_windows_on_display(int screen_index)
{
    std::vector<window_info*> screen_window_lst;
    for(int window_index = 0; window_index < window_lst.size(); ++window_index)
    {
        window_info *window = &window_lst[window_index];
        screen_info *screen = &display_lst[screen_index];
        if(window->x >= screen->x && window->x <= screen->x + screen->width)
            screen_window_lst.push_back(window);
    }

    return screen_window_lst;
}

void apply_layout_for_screen(int screen_index)
{
    std::vector<window_info*> screen_window_lst = get_all_windows_on_display(screen_index);
    screen_layout *layout_master = &screen_layout_lst[screen_index];
    int active_layout_index = layout_master->next_layout_index;
    layout_master->active_layout_index = active_layout_index;

    if(active_layout_index + 1 < layout_master->number_of_layouts)
        layout_master->next_layout_index = active_layout_index + 1; 
    else
        layout_master->next_layout_index = 0; 

    for(int window_index = 0; window_index < screen_window_lst.size(); ++window_index)
    {
        window_info *window = screen_window_lst[window_index];
        AXUIElementRef window_ref;
        if(get_window_ref(window, &window_ref))
        {
            if(window_index < layout_master->layouts[active_layout_index].layouts.size())
            {
                window_layout *layout = &layout_master->layouts[active_layout_index].layouts[window_index];
                window->layout = layout;
                window->layout_index = window_index;
                set_window_dimensions(window_ref, window, layout->x, layout->y, layout->width, layout->height); 
            }

            if(!windows_are_equal(&focused_window, window))
                CFRelease(window_ref);
        }
    }
}

void cycle_focused_window_layout(int screen_index, int shift)
{
    std::vector<window_info*> screen_window_lst = get_all_windows_on_display(screen_index);
    screen_layout *layout_master = &screen_layout_lst[screen_index];
    int active_layout_index = layout_master->active_layout_index;

    window_layout *focused_window_layout = get_layout_of_window(&focused_window);
    int focused_window_layout_index = focused_window.layout_index;

    if(!focused_window_layout)
        return;

    int swap_with_window_layout_index = focused_window_layout_index + shift;

    if(swap_with_window_layout_index < 0 || 
        swap_with_window_layout_index >= layout_master->number_of_layouts)
            return;

    int swap_with_window_index = -1;
    window_layout *swap_with_window_layout = &layout_master->layouts[active_layout_index].layouts[swap_with_window_layout_index];
    for(int window_index = 0; window_index < screen_window_lst.size(); ++window_index)
    {
        window_info *window = screen_window_lst[window_index];
        if(window_has_layout(window, swap_with_window_layout))
        {
            swap_with_window_index = window_index;
            break;
        }
    }

    if(swap_with_window_index == -1)
        return;

    AXUIElementRef window_ref;
    window_info *window = screen_window_lst[swap_with_window_index];
    if(get_window_ref(window, &window_ref))
    {
        set_window_dimensions(window_ref, 
                window, 
                focused_window_layout->x, 
                focused_window_layout->y, 
                focused_window_layout->width, 
                focused_window_layout->height); 

        set_window_dimensions(focused_window_ref, 
                &focused_window, 
                swap_with_window_layout->x, 
                swap_with_window_layout->y, 
                swap_with_window_layout->width, 
                swap_with_window_layout->height); 

        CFRelease(window_ref);
    }
}

void cycle_window_inside_layout(int screen_index)
{
    std::vector<window_info*> screen_window_lst = get_all_windows_on_display(screen_index);
    screen_layout *layout_master = &screen_layout_lst[screen_index];
    int active_layout_index = layout_master->active_layout_index;

    window_layout *focused_window_layout = get_layout_of_window(&focused_window);
    if(!focused_window_layout)
        return;

    int max_layout_tiles = layout_master->layouts[active_layout_index].layouts.size();
    if(screen_window_lst.size() <= max_layout_tiles)
        return;

    AXUIElementRef window_ref;
    window_info *window = screen_window_lst[screen_window_lst.size()-1];

    if(get_window_ref(window, &window_ref))
    {
        set_window_dimensions(window_ref,
                window,
                focused_window_layout->x,
                focused_window_layout->y,
                focused_window_layout->width,
                focused_window_layout->height);

            ProcessSerialNumber newpsn;
            GetProcessForPID(window->pid, &newpsn);

            AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
            AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
            AXUIElementPerformAction (window_ref, kAXRaiseAction);

            SetFrontProcessWithOptions(&newpsn, kSetFrontProcessFrontWindowOnly);

        if(focused_window_ref != NULL)
            CFRelease(focused_window_ref);
        
        focused_window_ref = window_ref;
        focused_window = *window;
        focused_psn = newpsn;
    }
}

bool get_window_ref(window_info *window, AXUIElementRef *window_ref)
{
    AXUIElementRef app = AXUIElementCreateApplication(window->pid);
    bool result = false;
    CFArrayRef app_window_lst;
    
    if(app)
    {
        AXError error = AXUIElementCopyAttributeValue(app, kAXWindowsAttribute, (CFTypeRef*)&app_window_lst);
        if(error == kAXErrorSuccess)
        {
            CFIndex do_not_free = -1;
            CFIndex app_window_count = CFArrayGetCount(app_window_lst);
            for(CFIndex window_index = 0; window_index < app_window_count; ++window_index)
            {
                AXUIElementRef app_window_ref = (AXUIElementRef)CFArrayGetValueAtIndex(app_window_lst, window_index);
                if(!result)
                {
                    AXValueRef temp;

                    CFStringRef app_window_title;
                    AXUIElementCopyAttributeValue(app_window_ref, kAXTitleAttribute, (CFTypeRef*)&app_window_title);

                    std::string app_window_name;
                    if(CFStringGetCStringPtr(app_window_title, kCFStringEncodingMacRoman))
                        app_window_name = CFStringGetCStringPtr(app_window_title, kCFStringEncodingMacRoman);
                    CFRelease(app_window_title);

                    CGSize app_window_size;
                    AXUIElementCopyAttributeValue(app_window_ref, kAXSizeAttribute, (CFTypeRef*)&temp);
                    AXValueGetValue(temp, kAXValueCGSizeType, &app_window_size);
                    CFRelease(temp);

                    CGPoint app_window_pos;
                    AXUIElementCopyAttributeValue(app_window_ref, kAXPositionAttribute, (CFTypeRef*)&temp);
                    AXValueGetValue(temp, kAXValueCGPointType, &app_window_pos);
                    CFRelease(temp);

                    if(window->x == app_window_pos.x && 
                            window->y == app_window_pos.y &&
                            window->width == app_window_size.width && 
                            window->height == app_window_size.height &&
                            window->name == app_window_name)
                    {
                        *window_ref = app_window_ref;
                        do_not_free = window_index;
                        result = true;
                    }

                }

                if(window_index != do_not_free)
                    CFRelease(app_window_ref);
            }
        }
        CFRelease(app);
    }

    return result;
}


window_layout get_window_layout_for_screen(int screen_index, const std::string &name)
{
    window_layout layout;
    layout.name = "invalid";

    screen_info *screen = &display_lst[screen_index];
    if(screen)
    {
        for(int layout_index = 0; layout_index < layout_lst.size(); ++layout_index)
        {
            if(layout_lst[layout_index].name == name)
            {
                layout = layout_lst[layout_index];
                break;
            }
        }

        if(name == "fullscreen")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    (screen->width - (layout.gap_vertical * 2)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "left vertical split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "right vertical split")
            set_window_layout_values(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "upper horizontal split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    (screen->width - (layout.gap_vertical * 2)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower horizontal split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    (screen->width - (layout.gap_vertical * 2)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "upper left split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower left split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "upper right split")
            set_window_layout_values(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower right split")
            set_window_layout_values(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
    }

    return layout;
}

void init_window_layouts()
{
    window_layout screen_vertical_split;
    screen_vertical_split.gap_x = 30;
    screen_vertical_split.gap_y = 40;
    screen_vertical_split.gap_vertical = 30;
    screen_vertical_split.gap_horizontal = 30;

    screen_vertical_split.name = "fullscreen";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "left vertical split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "right vertical split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper horizontal split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower horizontal split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper left split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower left split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper right split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower right split";
    layout_lst.push_back(screen_vertical_split);

    // Screen Layouts
    for(int screen_index = 0; screen_index < max_display_count; ++screen_index)
    {
        screen_layout layout_master;
        layout_master.active_layout_index = 0;
        layout_master.next_layout_index = 0;
        layout_master.number_of_layouts = 3;

        window_group_layout layout_tall;
        layout_tall.layouts.push_back(get_window_layout_for_screen(screen_index, "left vertical split"));
        layout_tall.layouts.push_back(get_window_layout_for_screen(screen_index, "upper right split"));
        layout_tall.layouts.push_back(get_window_layout_for_screen(screen_index, "lower right split"));
        layout_master.layouts.push_back(layout_tall);

        window_group_layout layout_vertical_split;
        layout_vertical_split.layouts.push_back(get_window_layout_for_screen(screen_index, "left vertical split"));
        layout_vertical_split.layouts.push_back(get_window_layout_for_screen(screen_index, "right vertical split"));
        layout_master.layouts.push_back(layout_vertical_split);

        window_group_layout layout_fullscreen;
        layout_fullscreen.layouts.push_back(get_window_layout_for_screen(screen_index, "fullscreen"));
        layout_master.layouts.push_back(layout_fullscreen);

        screen_layout_lst.push_back(layout_master);
    }

}

void set_window_dimensions(AXUIElementRef app_window, window_info *window, int x, int y, int width, int height)
{
    CGPoint window_pos = CGPointMake(x, y);
    CFTypeRef new_window_pos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&window_pos);

    CGSize window_size = CGSizeMake(width, height);
    CFTypeRef new_window_size = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&window_size);

    if(AXUIElementSetAttributeValue(app_window, kAXPositionAttribute, new_window_pos) != kAXErrorSuccess)
        std::cout << "failed to set new window position" << std::endl;
    if(AXUIElementSetAttributeValue(app_window, kAXSizeAttribute, new_window_size) != kAXErrorSuccess)
        std::cout << "failed to set new window size" << std::endl;

    window->x = window_pos.x;
    window->y = window_pos.y;
    window->width = window_size.width;
    window->height = window_size.height;

    get_layout_of_window(window);

    CFRelease(new_window_pos);
    CFRelease(new_window_size);
}

bool ffmouse_hotkey_commands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
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
            apply_layout_for_screen(get_display_of_window(&focused_window)->id);
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
                cycle_focused_window_screen(-1);

            if(keycode == kVK_ANSI_N)
                cycle_focused_window_screen(1);

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
        // Cycle window in layout
        if(keycode == kVK_ANSI_P || keycode == kVK_ANSI_N)
        {
            if(keycode == kVK_ANSI_P)
                cycle_focused_window_layout(get_display_of_window(&focused_window)->id, -1);

            if(keycode == kVK_ANSI_N)
                cycle_focused_window_layout(get_display_of_window(&focused_window)->id, 1);

            return true;
        }

        // Bring next window into this layout-tile
        if(keycode == kVK_Tab)
        {
            cycle_window_inside_layout(get_display_of_window(&focused_window)->id);
            //TODO: current-space-only alt tab effect for a
            // specific tile in the screen layout
        }
    }
    
    return false;
}

CGEventRef cgevent_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    if(type == kCGEventKeyDown)
    {
        CGEventFlags flags = CGEventGetFlags(event);
        bool cmd_key = (flags & kCGEventFlagMaskCommand) == kCGEventFlagMaskCommand;
        bool alt_key = (flags & kCGEventFlagMaskAlternate) == kCGEventFlagMaskAlternate;
        bool ctrl_key = (flags & kCGEventFlagMaskControl) == kCGEventFlagMaskControl;
        CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

        // Toggle keytap on | off
        if(ffmouse_hotkey_commands(cmd_key, ctrl_key, alt_key, keycode))
            return NULL;

        if(toggle_tap)
        {
            // Let system hotkeys pass through as normal
            if(system_hotkey_passthrough(cmd_key, ctrl_key, alt_key, keycode))
                return event;
            
            // capture custom hotkeys
            if(custom_hotkey_commands(cmd_key, ctrl_key, alt_key, keycode))
                return NULL;
        }
        else
        {
            // Re-enable keytap when spotlight is closed
            if ((cmd_key && keycode == kVK_Space)
                    || keycode == kVK_Return || keycode == kVK_ANSI_KeypadEnter)
            {
                toggle_tap = true;
                std::cout << "tap enabled" << std::endl;
            }
            return event;
        }

        CGEventSetIntegerValueField(event, kCGKeyboardEventAutorepeat, 0);
        CGEventPostToPSN(&focused_psn, event);
        return NULL;
    }
    else if(type == kCGEventMouseMoved)
        detect_window_below_cursor();

    return event;
}

void detect_window_below_cursor()
{
    window_lst.clear();

    CFArrayRef osx_window_list = CGWindowListCopyWindowInfo(osx_window_list_option, kCGNullWindowID);
    if(osx_window_list)
    {
        CFIndex osx_window_count = CFArrayGetCount(osx_window_list);
        for(CFIndex osx_window_index = 0; osx_window_index < osx_window_count; ++osx_window_index)
        {
            CFDictionaryRef elem = (CFDictionaryRef)CFArrayGetValueAtIndex(osx_window_list, osx_window_index);
            window_lst.push_back(window_info());
            CFDictionaryApplyFunction(elem, get_window_info, NULL);
        }
        CFRelease(osx_window_list);

        for(int i = 0; i < window_lst.size(); ++i)
        {
            if(is_window_below_cursor(&window_lst[i]))
            {
                if(!windows_are_equal(&focused_window, &window_lst[i]))
                {
                    ProcessSerialNumber newpsn;
                    GetProcessForPID(window_lst[i].pid, &newpsn);

                    focused_psn = newpsn;
                    focused_window = window_lst[i];
                    get_layout_of_window(&focused_window);

                    AXUIElementRef window_ref;
                    if(get_window_ref(&focused_window, &window_ref))
                    {
                        AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
                        AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
                        AXUIElementPerformAction (window_ref, kAXRaiseAction);

                        if(focused_window_ref != NULL)
                            CFRelease(focused_window_ref);

                        focused_window_ref = window_ref;
                    }

                    if(enable_auto_raise)
                        SetFrontProcessWithOptions(&focused_psn, kSetFrontProcessFrontWindowOnly);

                    std::cout << "Keyboard focus: " << focused_window.pid << std::endl;
                    if(focused_window.layout)
                        std::cout << focused_window.layout->name << std::endl;
                }
                break;
            }
        }
    }
}

bool is_window_below_cursor(window_info *window)
{
    CGEventRef event = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(event);
    CFRelease(event);
    if(cursor.x >= window->x && 
        cursor.x <= window->x + window->width && 
        cursor.y >= window->y && 
        cursor.y <= window->y + window->height)
            return true;
        
    return false;
}

window_info get_window_info_from_ref(AXUIElementRef window_ref)
{
    window_info info;

    CFStringRef window_title;
    AXValueRef temp;
    CGSize window_size;
    CGPoint window_pos;

    AXUIElementCopyAttributeValue(window_ref, kAXTitleAttribute, (CFTypeRef*)&window_title);
    if(CFStringGetCStringPtr(window_title, kCFStringEncodingMacRoman))
        info.name = CFStringGetCStringPtr(window_title, kCFStringEncodingMacRoman);
    CFRelease(window_title);

    AXUIElementCopyAttributeValue(window_ref, kAXSizeAttribute, (CFTypeRef*)&temp);
    AXValueGetValue(temp, kAXValueCGSizeType, &window_size);
    CFRelease(temp);

    AXUIElementCopyAttributeValue(window_ref, kAXPositionAttribute, (CFTypeRef*)&temp);
    AXValueGetValue(temp, kAXValueCGPointType, &window_pos);
    CFRelease(temp);

    info.width = window_size.width;
    info.height = window_size.height;

    return info;
}

void get_window_info(const void *key, const void *value, void *context)
{
    CFStringRef k = (CFStringRef)key;
    std::string key_str = CFStringGetCStringPtr(k, kCFStringEncodingMacRoman);

    CFTypeID id = CFGetTypeID(value);
    if(id == CFStringGetTypeID())
    {
        CFStringRef v = (CFStringRef)value;
        if(CFStringGetCStringPtr(v, kCFStringEncodingMacRoman))
        {
            std::string value_str = CFStringGetCStringPtr(v, kCFStringEncodingMacRoman);
            if(key_str == "kCGWindowName")
                window_lst[window_lst.size()-1].name = value_str;
        }
    }
    else if(id == CFNumberGetTypeID())
    {
        int myint;
        CFNumberRef v = (CFNumberRef)value;
        CFNumberGetValue(v, kCFNumberSInt64Type, &myint);
        if(key_str == "kCGWindowLayer")
            window_lst[window_lst.size()-1].layer = myint;
        else if(key_str == "kCGWindowOwnerPID")
            window_lst[window_lst.size()-1].pid = myint;
        else if(key_str == "X")
            window_lst[window_lst.size()-1].x = myint;
        else if(key_str == "Y")
            window_lst[window_lst.size()-1].y = myint;
        else if(key_str == "Width")
            window_lst[window_lst.size()-1].width = myint;
        else if(key_str == "Height")
            window_lst[window_lst.size()-1].height = myint;
    }
    else if(id == CFDictionaryGetTypeID())
    {
        CFDictionaryRef elem = (CFDictionaryRef)value;
        CFDictionaryApplyFunction(elem, get_window_info, NULL);
    } 
}

window_layout *get_layout_of_window(window_info *window)
{
    window_layout *result = NULL;

    int screen_id = get_display_of_window(window)->id;
    int active_layout_index = screen_layout_lst[screen_id].active_layout_index;
    int max_layout_size = screen_layout_lst[screen_id].layouts[active_layout_index].layouts.size();
    for(int layout_index = 0; layout_index < max_layout_size; ++layout_index)
    {
        window_layout *layout = &screen_layout_lst[screen_id].layouts[active_layout_index].layouts[layout_index];
        if(window_has_layout(window, layout))
        {
            window->layout = layout;
            window->layout_index = layout_index;
            result = layout;
            break;
        }
    }

    return result;
}

bool window_has_layout(window_info *window, window_layout *layout)
{
    if((window->x >= layout->x - 10 && window->x <= layout->x + 10) &&
        (window->y >= layout->y - 10 && window->y <= layout->y + 10 ) &&
        (window->width >= layout->width - 10 && window->width <= layout->width + 10) &&
        (window->height >= layout->height - 10 && window->height <= layout->height + 10))
            return true;

    return false;
}

bool windows_are_equal(window_info *window, window_info *match)
{
    if(window->x == match->x &&
        window->y == match->y &&
        window->width == match->width &&
        window->height == match->height &&
        window->name == match->name)
            return true;

    return false;
}

void fatal(const std::string &err)
{
    std::cout << err << std::endl;
    exit(1);
}

int main(int argc, char **argv)
{
    if(!check_privileges())
        request_privileges();

    CFMachPortRef event_tap;
    CGEventMask event_mask;
    CFRunLoopSourceRef run_loop_source;

    event_mask = ((1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) | (1 << kCGEventMouseMoved));
    event_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, event_mask, cgevent_callback, NULL);

    if(event_tap && CGEventTapIsEnabled(event_tap))
        std::cout << "tapping keys.." << std::endl;
    else
        fatal("could not tap keys, try running as root");

    get_active_displays();
    init_window_layouts();
    detect_window_below_cursor();
    
    run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);
    CGEventTapEnable(event_tap, true);
    CFRunLoopRun();
    return 0;
}
