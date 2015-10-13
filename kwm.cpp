#include "kwm.h"

std::vector<spaces_info> SpacesLst;
std::vector<screen_info> DisplayLst;
std::vector<window_info> WindowLst;
std::vector<screen_layout> ScreenLayoutLst;
std::vector<window_layout> LayoutLst;

ProcessSerialNumber FocusedPSN;
AXUIElementRef FocusedWindowRef;
window_info FocusedWindow;

bool ToggleTap = true;
bool EnableAutoraise = true;

uint32_t MaxDisplayCount = 5;
uint32_t ActiveDisplaysCount;
CGDirectDisplayID ActiveDisplays[5];

void Fatal(const std::string &Err)
{
    std::cout << Err << std::endl;
    exit(1);
}

bool CheckPrivileges()
{
    const void * Keys[] = { kAXTrustedCheckOptionPrompt };
    const void * Values[] = { kCFBooleanTrue };

    CFDictionaryRef Options;
    Options = CFDictionaryCreate(kCFAllocatorDefault, 
            Keys, Values, sizeof(Keys) / sizeof(*Keys),
            &kCFCopyStringDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

    return AXIsProcessTrustedWithOptions(Options);
}

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon)
{
    if(Type == kCGEventKeyDown)
    {
        CGEventFlags Flags = CGEventGetFlags(Event);
        bool CmdKey = (Flags & kCGEventFlagMaskCommand) == kCGEventFlagMaskCommand;
        bool AltKey = (Flags & kCGEventFlagMaskAlternate) == kCGEventFlagMaskAlternate;
        bool CtrlKey = (Flags & kCGEventFlagMaskControl) == kCGEventFlagMaskControl;
        CGKeyCode Keycode = (CGKeyCode)CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);

        // Toggle keytap on | off
        if(KwmHotkeyCommands(CmdKey, CtrlKey, AltKey, Keycode))
            return NULL;

        // Let system hotkeys pass through as normal
        if(SystemHotkeyPassthrough(CmdKey, CtrlKey, AltKey, Keycode))
            return Event;
            
        // capture custom hotkeys
        if(CustomHotkeyCommands(CmdKey, CtrlKey, AltKey, Keycode))
            return NULL;

        if(ToggleTap)
        {
            CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
            CGEventPostToPSN(&FocusedPSN, Event);
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
            return Event;
        }

    }
    else if(Type == kCGEventMouseMoved)
        DetectWindowBelowCursor();

    return Event;
}

int main(int argc, char **argv)
{
    if(!CheckPrivileges())
        Fatal("Could not access OSX Accessibility!"); 

    CFMachPortRef EventTap;
    CGEventMask EventMask;
    CFRunLoopSourceRef RunLoopSource;

    EventMask = ((1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) | (1 << kCGEventMouseMoved));
    EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, EventMask, CGEventCallback, NULL);

    if(!EventTap || !CGEventTapIsEnabled(EventTap))
        Fatal("could not tap keys, try running as root");

    GetActiveDisplays();
    InitWindowLayouts();
    GetActiveSpaces();
    DetectWindowBelowCursor();

    RunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, EventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), RunLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(EventTap, true);
    CFRunLoopRun();
    return 0;
}
