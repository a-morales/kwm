#include "ffmouse.h"

static const std::string OSASCRIPT_START = "osascript -e 'tell application \"System Events\" to tell process \"";
static const std::string OSASCRIPT_MID = "\" to perform action \"AXRaise\" of (first window whose name contains \"";
static const std::string OSASCRIPT_END = "\")'";

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

static pid_t pid;
static ProcessSerialNumber psn;
static std::vector<screen_info> display_lst;
static std::vector<app_info> window_lst;
static std::vector<window_layout> layout_lst;
static app_info focused_window;
static bool toggle_tap = true;

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
    if(AXMakeProcessTrusted(CFSTR("/usr/bin/accessability")) != kAXErrorSuccess)
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
        {
            layout.x = screen->x + layout.gap_x;
            layout.y = screen->y + layout.gap_y;
            layout.width = (screen->width - (layout.gap_vertical * 2));
            layout.height = (screen->height - (layout.gap_y * 1.5f));
        }
        else if(name == "left vertical split")
        {
            layout.x = screen->x + layout.gap_x;
            layout.y = screen->y + layout.gap_y;
            layout.width = ((screen->width / 2) - (layout.gap_vertical * 1.5f));
            layout.height = (screen->height - (layout.gap_y * 1.5f));
        }
        else if(name == "right vertical split")
        {
            layout.x = screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f));
            layout.y = screen->y + layout.gap_y;
            layout.width = ((screen->width / 2) - (layout.gap_vertical * 1.5f));
            layout.height = (screen->height - (layout.gap_y * 1.5f));
        }
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

    if(cmd_key && ctrl_key && !alt_key)
    {
        // Hammerspoon hotkeys -> window movement
        if(keycode == kVK_ANSI_P
            || (keycode == kVK_ANSI_N)
            || (keycode == kVK_ANSI_H)
            || (keycode == kVK_ANSI_J)
            || (keycode == kVK_ANSI_K)
            || (keycode == kVK_ANSI_L))
            return true;
    }

    if(cmd_key && alt_key && ctrl_key)
    {
        // Hammerspoon hotkeys -> config reload
        if(keycode == kVK_ANSI_R)
            return true;

        // Hammerspoon hotkeys -> window resize
        if(keycode == kVK_UpArrow
            || (keycode == kVK_DownArrow)
            || (keycode == kVK_ANSI_H)
            || (keycode == kVK_ANSI_J)
            || (keycode == kVK_ANSI_K)
            || (keycode == kVK_ANSI_L)
            || (keycode == kVK_ANSI_P)
            || (keycode == kVK_SPECIAL_Å)
            || (keycode == kVK_SPECIAL_Ø)
            || (keycode == kVK_SPECIAL_Æ))
            return true;

        // Hammerspoon hotkeys -> launch app
        if(keycode == kVK_ANSI_1)
            return true;
    }
    return false;
}

bool custom_hotkey_commands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode)
{
    if(cmd_key && alt_key && ctrl_key)
    {
        // YTD - Media Player Controls
        std::string sys_command = "";
        if(keycode == kVK_ANSI_Z)
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

        if(keycode == kVK_LeftArrow)
        {
            window_layout layout = get_window_layout_for_screen("left vertical split");
            if(layout.name != "invalid")
                set_window_dimensions(layout.x, layout.y, layout.width, layout.height);
            return true;
        }
        else if(keycode == kVK_RightArrow)
        {
            window_layout layout = get_window_layout_for_screen("right vertical split");
            if(layout.name != "invalid")
                set_window_dimensions(layout.x, layout.y, layout.width, layout.height);
            return true;
        }
        else if(keycode == kVK_ANSI_M)
        {
            window_layout layout = get_window_layout_for_screen("fullscreen");
            if(layout.name != "invalid")
                set_window_dimensions(layout.x, layout.y, layout.width, layout.height);
            return true;
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

    CFArrayRef osx_window_list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if(osx_window_list)
    {
        CFIndex osx_window_count = CFArrayGetCount(osx_window_list);
        for(CFIndex osx_window_index = 0; osx_window_index < osx_window_count; ++osx_window_index)
        {
            CFDictionaryRef elem = (CFDictionaryRef)CFArrayGetValueAtIndex(osx_window_list, osx_window_index);
            window_lst.push_back(app_info());
            CFDictionaryApplyFunction(elem, get_window_info, NULL);
        }

        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        //std::cout << "Mouse Pos: " << cursor.x << ", " << cursor.y << std::endl;
        for(int i = 0; i < window_lst.size(); ++i)
        {
            if(window_lst[i].layer == 0)
            {
                if(cursor.x >= window_lst[i].x && cursor.x <= window_lst[i].x + window_lst[i].width
                        && cursor.y >= window_lst[i].y && cursor.y <= window_lst[i].y + window_lst[i].height)
                {
                    ProcessSerialNumber newpsn;
                    GetProcessForPID(window_lst[i].pid, &newpsn);
                    pid = window_lst[i].pid;
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

    get_active_displays();
    init_window_layouts();

    detect_window_below_cursor();

    CFMachPortRef event_tap;
    CGEventMask event_mask;
    CFRunLoopSourceRef run_loop_source;

    event_mask = ((1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) | (1 << kCGEventMouseMoved));
    event_tap = CGEventTapCreate( kCGSessionEventTap, kCGHeadInsertEventTap, 0, event_mask, cgevent_callback, NULL);

    if(event_tap && CGEventTapIsEnabled(event_tap))
        std::cout << "tapping keys.." << std::endl;
    else
        fatal("could not tap keys, try running as root");

    run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);
    CGEventTapEnable(event_tap, true);
    CFRunLoopRun();
    return 0;
}
