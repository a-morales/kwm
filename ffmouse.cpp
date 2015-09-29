#include "ffmouse.h"

static const std::string OSASCRIPT_START = "osascript -e 'tell application \"System Events\" to tell process \"";
static const std::string OSASCRIPT_MID = "\" to perform action \"AXRaise\" of (first window whose name contains \"";
static const std::string OSASCRIPT_END = "\")'";

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

static pid_t pid;
static ProcessSerialNumber psn;
static std::vector<app_info> window_lst;
static int window_lst_focus_index;
static std::string window_lst_focus_name = "<no focus>";;
static bool toggle_tap = true;

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
        if(keycode == kVK_LeftArrow
            || (keycode == kVK_UpArrow)
            || (keycode == kVK_DownArrow)
            || (keycode == kVK_RightArrow)
            || (keycode == kVK_ANSI_M)
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
    }
    return false;
}

CGEventRef cgevent_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    if(type != kCGEventKeyDown && type != kCGEventKeyUp && type != kCGEventMouseMoved)
        return event;

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
    else if(type == kCGEventKeyUp)
        return event;
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
        for(CFIndex i = 0; i < osx_window_count; ++i)
        {
            CFDictionaryRef elem = (CFDictionaryRef)CFArrayGetValueAtIndex(osx_window_list, i);
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
                    if(window_lst_focus_name != window_lst[i].name || pid != window_lst[i].pid)
                    {
                        window_lst_focus_name = window_lst[i].name;
                        window_lst_focus_index = i;

                        // Strip single quotes and remaining part if found,
                        // because  it breaks applescript syntax
                        std::string win_title = window_lst[window_lst_focus_index].name;
                        if(win_title != "")
                        {
                            std::size_t pos = win_title.find("'");
                            if(pos != std::string::npos)
                                win_title.erase(pos);

                            std::string applescript_cmd = OSASCRIPT_START +
                                window_lst[window_lst_focus_index].owner + OSASCRIPT_MID +
                                win_title + OSASCRIPT_END;
                            system(applescript_cmd.c_str());
                        }

                        if(pid != window_lst[i].pid)
                        {
                            pid = window_lst[i].pid;
                            GetProcessForPID(pid, &psn);
                            std::cout << "Keyboard focus: " << pid << std::endl;
                        }
                    }
                    break;
                }

                /*
                std::cout << "Owner: " << window_lst[i].owner << std::endl;
                std::cout << "Name: " << window_lst[i].name << std::endl;
                std::cout << "PID: " << window_lst[i].pid << std::endl;
                std::cout << "Layer: " << window_lst[i].layer << std::endl;
                std::cout << "X: " << window_lst[i].x << std::endl;
                std::cout << "Y: " << window_lst[i].y << std::endl;
                std::cout << "Width: " << window_lst[i].width << std::endl;
                std::cout << "Height: " << window_lst[i].height << std::endl;
                */
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
