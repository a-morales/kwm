#include "kwm.h"

std::vector<spaces_info> spaces_lst;
std::vector<screen_info> display_lst;
std::vector<window_info> window_lst;
std::vector<screen_layout> screen_layout_lst;
std::vector<window_layout> layout_lst;

ProcessSerialNumber focused_psn;
AXUIElementRef focused_window_ref;
window_info focused_window;

bool toggle_tap = true;
bool enable_auto_raise = true;

uint32_t max_display_count = 5;
uint32_t active_displays_count;
CGDirectDisplayID active_displays[5];

void fatal(const std::string &err)
{
    std::cout << err << std::endl;
    exit(1);
}

bool check_privileges()
{
    const void * keys[] = { kAXTrustedCheckOptionPrompt };
    const void * values[] = { kCFBooleanTrue };

    CFDictionaryRef options;
    options = CFDictionaryCreate(kCFAllocatorDefault, 
            keys, values, sizeof(keys) / sizeof(*keys),
            &kCFCopyStringDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

    return AXIsProcessTrustedWithOptions(options);
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
        if(kwm_hotkey_commands(cmd_key, ctrl_key, alt_key, keycode))
            return NULL;

        // Let system hotkeys pass through as normal
        if(system_hotkey_passthrough(cmd_key, ctrl_key, alt_key, keycode))
            return event;
            
        // capture custom hotkeys
        if(custom_hotkey_commands(cmd_key, ctrl_key, alt_key, keycode))
            return NULL;

        if(toggle_tap)
        {
            CGEventSetIntegerValueField(event, kCGKeyboardEventAutorepeat, 0);
            CGEventPostToPSN(&focused_psn, event);
            return NULL;
        }
        else
        {
            /* Re-enable keytap when spotlight is closed
            
            if ((cmd_key && keycode == kVK_Space)
                    || keycode == kVK_Return || keycode == kVK_ANSI_KeypadEnter)
            {
                toggle_tap = true;
                std::cout << "tap enabled" << std::endl;
            }

            */
            return event;
        }

    }
    else if(type == kCGEventMouseMoved)
        detect_window_below_cursor();

    return event;
}

int main(int argc, char **argv)
{
    if(!check_privileges())
        fatal("Could not access OSX Accessibility!"); 

    CFMachPortRef event_tap;
    CGEventMask event_mask;
    CFRunLoopSourceRef run_loop_source;

    event_mask = ((1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) | (1 << kCGEventMouseMoved));
    event_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, event_mask, cgevent_callback, NULL);

    if(!event_tap || !CGEventTapIsEnabled(event_tap))
        fatal("could not tap keys, try running as root");

    get_active_displays();
    init_window_layouts();
    get_active_spaces();
    detect_window_below_cursor();

    run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);
    CGEventTapEnable(event_tap, true);
    CFRunLoopRun();
    return 0;
}
