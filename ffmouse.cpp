#include "ffmouse.h"

static const std::string OSASCRIPT_START = "osascript -e 'tell application \"System Events\" to tell process \"";
static const std::string OSASCRIPT_MID = "\" to perform action \"AXRaise\" of (first window whose name contains \"";
static const std::string OSASCRIPT_END = "\")'";

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

static std::vector<screen_info> display_lst;
static std::vector<window_info> window_lst;
static std::vector<window_layout> layout_lst;
static ProcessSerialNumber psn;
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

void send_window_to_prev_screen()
{
    screen_info *cur_screen = get_display_of_window();
    int new_screen_index = (cur_screen->id - 1 < 0) ? active_displays_count - 1 : cur_screen->id - 1;

    screen_info *screen = &display_lst[new_screen_index];
    set_window_dimensions(screen->x + 10, 
            screen->y + 10,
            focused_window.width,
            focused_window.height);
}

void send_window_to_next_screen()
{
    screen_info *cur_screen = get_display_of_window();
    int new_screen_index = (cur_screen->id + 1 >= active_displays_count) ? 0 : cur_screen->id + 1;

    screen_info *screen = &display_lst[new_screen_index];
    set_window_dimensions(screen->x + 10, 
            screen->y + 10,
            focused_window.width,
            focused_window.height);
}

screen_info *get_display_of_window()
{
    for(int display_index = 0; display_index < active_displays_count; ++display_index)
    {
        screen_info *screen = &display_lst[display_index];
        if(focused_window.x >= screen->x && focused_window.x <= screen->x + screen->width)
            return screen;
    }

    return NULL;
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

std::vector<window_info> get_all_windows_on_display(int screen_index)
{
    std::vector<window_info> screen_window_lst;
    for(int window_index = 0; window_index < window_lst.size(); ++window_index)
    {
        window_info *window = &window_lst[window_index];
        screen_info *screen = get_display_of_window(window);
        if(window->x >= screen->x && window->x <= screen->x + screen->width)
            screen_window_lst.push_back(*window);
    }

    return screen_window_lst;
}

window_layout get_window_layout_for_screen(const std::string &name)
{
    window_layout layout;
    layout.name = "invalid";

    screen_info *screen = get_display_of_window();
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
                    (screen->height - (layout.gap_y * 1.0f)));
        else if(name == "right vertical split")
            set_window_layout_values(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    (screen->height - (layout.gap_y * 1.0f)));
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
}

void set_window_dimensions(int x, int y, int width, int height)
{
    AXUIElementRef app = AXUIElementCreateApplication(focused_window.pid);
    if(app)
    {
        AXUIElementRef app_window = NULL;

        AXError error = AXUIElementCopyAttributeValue(app, kAXFocusedWindowAttribute, (CFTypeRef*)&app_window);
        if(error == kAXErrorSuccess)
        {
            std::cout << "target window: " << focused_window.name << std::endl;

            CGPoint window_pos = CGPointMake(x, y);
            CFTypeRef new_window_pos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&window_pos);

            CGSize window_size = CGSizeMake(width, height);
            CFTypeRef new_window_size = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&window_size);

            if(AXUIElementSetAttributeValue(app_window, kAXPositionAttribute, new_window_pos) != kAXErrorSuccess)
                std::cout << "failed to set new window position" << std::endl;
            if(AXUIElementSetAttributeValue(app_window, kAXSizeAttribute, new_window_size) != kAXErrorSuccess)
                std::cout << "failed to set new window size" << std::endl;

            focused_window.x = window_pos.x;
            focused_window.y = window_pos.y;
            focused_window.width = window_size.width;
            focused_window.height = window_size.height;

            CFRelease(new_window_pos);
            CFRelease(new_window_size);
            CFRelease(app_window);
        }

        CFRelease(app);
    }
}

bool toggle_tap_hotkey(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
{
    if(cmd_key && alt_key && ctrl_key)
    {
        if(keycode == kVK_ANSI_T)
        {
            toggle_tap = !toggle_tap;
            std::cout << (toggle_tap ? "tap enabled" : "tap disabled") << std::endl;
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

        // Window Layout
        window_layout layout;
        layout.name = "invalid";
        if(keycode == kVK_ANSI_M)
            layout = get_window_layout_for_screen("fullscreen");
        else if(keycode == kVK_LeftArrow)
            layout = get_window_layout_for_screen("left vertical split");
        else if(keycode == kVK_RightArrow)
            layout = get_window_layout_for_screen("right vertical split");
        else if(keycode == kVK_UpArrow)
            layout = get_window_layout_for_screen("upper horizontal split");
        else if(keycode == kVK_DownArrow)
            layout = get_window_layout_for_screen("lower horizontal split");
        else if(keycode == kVK_ANSI_P)
            layout = get_window_layout_for_screen("upper left split");
        else if(keycode == kVK_SPECIAL_Ø)
            layout = get_window_layout_for_screen("lower left split");
        else if(keycode == kVK_SPECIAL_Å)
            layout = get_window_layout_for_screen("upper right split");
        else if(keycode == kVK_SPECIAL_Æ)
            layout = get_window_layout_for_screen("lower right split");

        if(layout.name != "invalid")
        {
            set_window_dimensions(layout.x, layout.y, layout.width, layout.height);
            return true;
        }

        // Window Resize
        if(keycode == kVK_ANSI_H)
            set_window_dimensions(focused_window.x, 
                    focused_window.y, 
                    focused_window.width - 10, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_J)
            set_window_dimensions(focused_window.x, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height + 10);
        else if(keycode == kVK_ANSI_K)
            set_window_dimensions(focused_window.x, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height - 10);
        else if(keycode == kVK_ANSI_L)
            set_window_dimensions(focused_window.x, 
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
                send_window_to_prev_screen();

            if(keycode == kVK_ANSI_N)
                send_window_to_next_screen();

            return true;
        }

        // Move Window
        if(keycode == kVK_ANSI_H)
            set_window_dimensions(focused_window.x - 10, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_J)
            set_window_dimensions(focused_window.x, 
                    focused_window.y + 10, 
                    focused_window.width, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_K)
            set_window_dimensions(focused_window.x, 
                    focused_window.y - 10, 
                    focused_window.width, 
                    focused_window.height);
        else if(keycode == kVK_ANSI_L)
            set_window_dimensions(focused_window.x + 10, 
                    focused_window.y, 
                    focused_window.width, 
                    focused_window.height);
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
        if(toggle_tap_hotkey(cmd_key, ctrl_key, alt_key, keycode))
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
        CGEventPostToPSN(&psn, event);
        return NULL;
    }
    else if(type == kCGEventMouseMoved)
        detect_window_below_cursor();

    return event;
}

void detect_window_below_cursor()
{
    window_lst.clear();

    CGWindowListOption osx_window_list_option = kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements;
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

        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        for(int i = 0; i < window_lst.size(); ++i)
        {
            if(window_lst[i].layer == 0)
            {
                if(cursor.x >= window_lst[i].x && cursor.x <= window_lst[i].x + window_lst[i].width
                        && cursor.y >= window_lst[i].y && cursor.y <= window_lst[i].y + window_lst[i].height)
                {
                    ProcessSerialNumber newpsn;
                    GetProcessForPID(window_lst[i].pid, &newpsn);
                    psn = newpsn;

                    if(focused_window.name != window_lst[i].name)
                    {
                        focused_window = window_lst[i];

                        // Strip single quotes and remaining part if found,
                        // because  it breaks applescript syntax
                        std::string win_title = focused_window.name;
                        if(win_title != "")
                        {
                            std::size_t pos = win_title.find("'");
                            if(pos != std::string::npos)
                                win_title.erase(pos);

                            std::string applescript_cmd = OSASCRIPT_START +
                                focused_window.owner + OSASCRIPT_MID +
                                win_title + OSASCRIPT_END;

                            system(applescript_cmd.c_str());
                            if(enable_auto_raise)
                                SetFrontProcessWithOptions(&psn, kSetFrontProcessFrontWindowOnly);
                        }

                        std::cout << "Keyboard focus: " << focused_window.pid << std::endl;
                    }
                    break;
                }
            }
        }
    }
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
            else if(key_str == "kCGWindowOwnerName")
                window_lst[window_lst.size()-1].owner = value_str;
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
        CFRelease(elem);
    } 
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
    event_tap = CGEventTapCreate( kCGSessionEventTap, kCGHeadInsertEventTap, 0, event_mask, cgevent_callback, NULL);

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
