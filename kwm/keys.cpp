#include "kwm.h"

extern std::vector<hotkey> KwmHotkeys;
extern kwm_prefix KWMPrefix;
extern kwm_focus KWMFocus;

bool HotkeysAreEqual(hotkey *A, hotkey *B)
{
    if(A && B)
    {
        if(A->Mod.CmdKey == B->Mod.CmdKey &&
           A->Mod.CtrlKey == B->Mod.CtrlKey &&
           A->Mod.AltKey == B->Mod.AltKey &&
           A->Mod.ShiftKey == B->Mod.ShiftKey &&
           A->Key == B->Key)
            return true;
    }

    return false;
}

bool KwmMainHotkeyTrigger(CGEventRef *Event)
{
    modifiers Mod = {};
    CGEventFlags Flags = CGEventGetFlags(*Event);
    Mod.CmdKey = (Flags & kCGEventFlagMaskCommand) == kCGEventFlagMaskCommand;
    Mod.AltKey = (Flags & kCGEventFlagMaskAlternate) == kCGEventFlagMaskAlternate;
    Mod.CtrlKey = (Flags & kCGEventFlagMaskControl) == kCGEventFlagMaskControl;
    Mod.ShiftKey = (Flags & kCGEventFlagMaskShift) == kCGEventFlagMaskShift;
    CGKeyCode Keycode = (CGKeyCode)CGEventGetIntegerValueField(*Event, kCGKeyboardEventKeycode);

    if(KWMPrefix.Enabled && KwmIsPrefixKey(&KWMPrefix.Key, &Mod, Keycode))
    {
        KWMPrefix.Active = true;
        KWMPrefix.Time = std::chrono::steady_clock::now();
        return true;
    }

    if(!KWMPrefix.Enabled || KWMPrefix.Active)
    {
        if(KWMPrefix.Active)
        {
            kwm_time_point NewPrefixTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> Diff = NewPrefixTime - KWMPrefix.Time;
            if(Diff.count() > KWMPrefix.Timeout)
            {
                KWMPrefix.Active = false;
                return false;
            }
        }

        // Hotkeys bound using `kwmc bind keys command`
        if(KwmExecuteHotkey(Mod, Keycode))
        {
            if(KWMPrefix.Active)
                KWMPrefix.Time = std::chrono::steady_clock::now();

            return true;
        }

        // Code for live-coded hotkey system; hotkeys.cpp
        if(KwmRunLiveCodeHotkeySystem(Event, &Mod, Keycode))
        {
            if(KWMPrefix.Active)
                KWMPrefix.Time = std::chrono::steady_clock::now();

            return true;
        }
    }
    
    return false;
}

bool KwmIsPrefixKey(hotkey *PrefixKey, modifiers *Mod, CGKeyCode Keycode)
{
    hotkey TempHotkey;
    TempHotkey.Mod = *Mod;
    TempHotkey.Key = Keycode;

    return HotkeysAreEqual(PrefixKey, &TempHotkey);
}

bool KwmExecuteHotkey(modifiers Mod, CGKeyCode Keycode)
{
    hotkey Hotkey = {};
    if(HotkeyExists(Mod, Keycode, &Hotkey))
    {
        if(Hotkey.Command.empty())
            return true;

        if(Hotkey.IsSystemCommand)
            system(Hotkey.Command.c_str());
        else
            KwmInterpretCommand(Hotkey.Command, 0);

        return true;
    }

    return false;
}

bool HotkeyExists(modifiers Mod, CGKeyCode Keycode, hotkey *Hotkey)
{
    hotkey TempHotkey;
    TempHotkey.Mod = Mod;
    TempHotkey.Key = Keycode;

    for(int HotkeyIndex = 0; HotkeyIndex < KwmHotkeys.size(); ++HotkeyIndex)
    {
        if(HotkeysAreEqual(&KwmHotkeys[HotkeyIndex], &TempHotkey))
        {
            if(Hotkey)
                *Hotkey = KwmHotkeys[HotkeyIndex];

            return true;
        }
    }
    
    return false;
}

bool KwmParseHotkey(std::string KeySym, std::string Command, hotkey *Hotkey)
{
    std::vector<std::string> KeyTokens = SplitString(KeySym, '-');
    if(KeyTokens.size() != 2)
        return false;

    std::vector<std::string> Modifiers = SplitString(KeyTokens[0], '+');
    for(int ModIndex = 0; ModIndex < Modifiers.size(); ++ModIndex)
    {
        if(Modifiers[ModIndex] == "cmd")
            Hotkey->Mod.CmdKey = true;
        else if(Modifiers[ModIndex] == "alt")
            Hotkey->Mod.AltKey = true;
        else if(Modifiers[ModIndex] == "ctrl")
            Hotkey->Mod.CtrlKey = true;
        else if(Modifiers[ModIndex] == "shift")
            Hotkey->Mod.ShiftKey = true;
    }

    Hotkey->IsSystemCommand = IsPrefixOfString(Command, "sys");
    Hotkey->Command = Command;

    CGKeyCode Keycode;
    bool Result = GetLayoutIndependentKeycode(KeyTokens[1], &Keycode);
    if(!Result)
        Result = KeycodeForChar(KeyTokens[1][0], &Keycode);

    Hotkey->Key = Keycode;
    return Result;
}

void KwmSetGlobalPrefix(std::string KeySym)
{
    hotkey Hotkey = {};
    if(KwmParseHotkey(KeySym, "", &Hotkey))
    {
        KWMPrefix.Key = Hotkey;
        KWMPrefix.Active = false;
        KWMPrefix.Enabled = true;
    }
}

void KwmSetGlobalPrefixTimeout(double Timeout)
{
    KWMPrefix.Timeout = Timeout;
}

void KwmAddHotkey(std::string KeySym, std::string Command)
{
    hotkey Hotkey = {};
    if(KwmParseHotkey(KeySym, Command, &Hotkey) &&
       !HotkeyExists(Hotkey.Mod, Hotkey.Key, NULL))
            KwmHotkeys.push_back(Hotkey);
}

void KwmRemoveHotkey(std::string KeySym)
{
    hotkey NewHotkey = {};
    if(KwmParseHotkey(KeySym, "", &NewHotkey))
    {
        for(int HotkeyIndex = 0; HotkeyIndex < KwmHotkeys.size(); ++HotkeyIndex)
        {
            if(HotkeysAreEqual(&KwmHotkeys[HotkeyIndex], &NewHotkey))
            {
                KwmHotkeys.erase(KwmHotkeys.begin() + HotkeyIndex);
                break;
            }
        }
    }
}

bool GetLayoutIndependentKeycode(std::string Key, CGKeyCode *Keycode)
{
    bool Result = true;

    if(Key == "return")
        *Keycode = 0x24;
    else if(Key == "tab")
        *Keycode = 0x30;
    else if(Key == "space")
        *Keycode = 0x31;
    else if(Key == "delete")
        *Keycode = 0x33;
    else if(Key == "escape")
        *Keycode =  0x35;
    else if(Key == "leftarrow")
        *Keycode =  0x7B;
    else if(Key == "rightarrow")
        *Keycode =  0x7C;
    else if(Key == "uparrow")
        *Keycode = 0x7E;
    else if(Key == "downarrow")
        *Keycode =  0x7D;
    else
        Result = false;

    return Result;
}

CFStringRef KeycodeToString(CGKeyCode Keycode)
{
    TISInputSourceRef Keyboard = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
    CFDataRef Uchr = (CFDataRef)TISGetInputSourceProperty(Keyboard, kTISPropertyUnicodeKeyLayoutData);
    const UCKeyboardLayout *KeyboardLayout = (const UCKeyboardLayout*)CFDataGetBytePtr(Uchr);

    if(KeyboardLayout)
    {
        UInt32 DeadKeyState = 0;
        UniCharCount MaxStringLength = 255;
        UniCharCount ActualStringLength = 0;
        UniChar UnicodeString[MaxStringLength];

        OSStatus Status = UCKeyTranslate(KeyboardLayout, Keycode,
                                         kUCKeyActionDown, 0,
                                         LMGetKbdType(), 0,
                                         &DeadKeyState,
                                         MaxStringLength,
                                         &ActualStringLength, 
                                         UnicodeString);

        if (ActualStringLength == 0 && DeadKeyState)
        {
            Status = UCKeyTranslate(KeyboardLayout, kVK_Space,
                                    kUCKeyActionDown, 0,
                                    LMGetKbdType(), 0,
                                    &DeadKeyState,
                                    MaxStringLength,
                                    &ActualStringLength,
                                    UnicodeString);
        }

        if(ActualStringLength > 0 && Status == noErr)
            return CFStringCreateWithCharacters(NULL, UnicodeString, ActualStringLength);
    }

    return NULL;
}

bool KeycodeForChar(char Key, CGKeyCode *Keycode)
{
    static CFMutableDictionaryRef CharToCodeDict = NULL;

    bool Result = true;
    UniChar Character = Key;
    CFStringRef CharStr;

    if (!CharToCodeDict)
    {
        CharToCodeDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 128,
                                                   &kCFCopyStringDictionaryKeyCallBacks, NULL);
        if (!CharToCodeDict)
            return false;

        for (size_t KeyIndex = 0; KeyIndex < 128; ++KeyIndex)
        {
            CFStringRef KeyString = KeycodeToString((CGKeyCode)KeyIndex);
            if (KeyString != NULL)
            {
                CFDictionaryAddValue(CharToCodeDict, KeyString, (const void *)KeyIndex);
                CFRelease(KeyString);
            }
        }
    }

    CharStr = CFStringCreateWithCharacters(kCFAllocatorDefault, &Character, 1);
    if (!CFDictionaryGetValueIfPresent(CharToCodeDict, CharStr, (const void **)Keycode))
        Result = false;
        
    CFRelease(CharStr);
    return Result;
}

void KwmEmitKeystrokes(std::string Text)
{
    CFStringRef TextRef = CFStringCreateWithCString(NULL, Text.c_str(), kCFStringEncodingMacRoman);
    CGEventRef EventKeyDown = CGEventCreateKeyboardEvent(NULL, 0, true);
    CGEventRef EventKeyUp = CGEventCreateKeyboardEvent(NULL, 0, false);

    UniChar OutputBuffer;
    for (int CharIndex = 0; CharIndex < Text.size(); ++CharIndex)
    {
        CFStringGetCharacters(TextRef, CFRangeMake(CharIndex, 1), &OutputBuffer);

        CGEventSetFlags(EventKeyDown, 0);
        CGEventKeyboardSetUnicodeString(EventKeyDown, 1, &OutputBuffer);
        CGEventPostToPSN(&KWMFocus.PSN, EventKeyDown);

        CGEventSetFlags(EventKeyUp, 0);
        CGEventKeyboardSetUnicodeString(EventKeyUp, 1, &OutputBuffer);
        CGEventPostToPSN(&KWMFocus.PSN, EventKeyUp);
    }

    CFRelease(EventKeyUp);
    CFRelease(EventKeyDown);
    CFRelease(TextRef);
}

