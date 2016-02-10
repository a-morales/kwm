#include "keys.h"
#include "kwm.h"
#include "helpers.h"
#include "interpreter.h"
#include "border.h"

extern kwm_focus KWMFocus;
extern kwm_hotkeys KWMHotkeys;
extern kwm_thread KWMThread;
extern kwm_border PrefixBorder;

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

    if(KWMHotkeys.Prefix.Enabled && KwmIsPrefixKey(&KWMHotkeys.Prefix.Key, &Mod, Keycode))
    {
        KWMHotkeys.Prefix.Active = true;
        KWMHotkeys.Prefix.Time = std::chrono::steady_clock::now();
        if(PrefixBorder.Enabled)
            UpdateBorder("focused");

        return true;
    }

    return KwmExecuteHotkey(Mod, Keycode);
}

bool KwmIsPrefixKey(hotkey *PrefixKey, modifiers *Mod, CGKeyCode Keycode)
{
    hotkey TempHotkey;
    TempHotkey.Mod = *Mod;
    TempHotkey.Key = Keycode;

    return HotkeysAreEqual(PrefixKey, &TempHotkey);
}

void CheckPrefixTimeout()
{
    if(KWMHotkeys.Prefix.Active)
    {
        kwm_time_point NewPrefixTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> Diff = NewPrefixTime - KWMHotkeys.Prefix.Time;
        if(Diff.count() > KWMHotkeys.Prefix.Timeout)
        {
            KWMHotkeys.Prefix.Active = false;
            if(PrefixBorder.Enabled)
                UpdateBorder("focused");
        }
    }
}

bool IsHotkeyStateReqFulfilled(hotkey *Hotkey)
{
    if(Hotkey->State == HotkeyStateInclude && KWMFocus.Window)
    {
        for(std::size_t AppIndex = 0; AppIndex < Hotkey->List.size(); ++AppIndex)
        {
            if(KWMFocus.Window->Owner == Hotkey->List[AppIndex])
                return true;
        }

        return false;
    }
    else if(Hotkey->State == HotkeyStateExclude && KWMFocus.Window)
    {
        for(std::size_t AppIndex = 0; AppIndex < Hotkey->List.size(); ++AppIndex)
        {
            if(KWMFocus.Window->Owner == Hotkey->List[AppIndex])
                return false;
        }
    }

    return true;
}

bool KwmExecuteHotkey(modifiers Mod, CGKeyCode Keycode)
{
    hotkey Hotkey = {};
    if(HotkeyExists(Mod, Keycode, &Hotkey))
    {
        if(KWMHotkeys.Prefix.Enabled)
        {
            CheckPrefixTimeout();
            if((Hotkey.Prefixed || KWMHotkeys.Prefix.Global) &&
                !KWMHotkeys.Prefix.Active)
                    return false;

            if((Hotkey.Prefixed || KWMHotkeys.Prefix.Global) &&
                KWMHotkeys.Prefix.Active)
                    KWMHotkeys.Prefix.Time = std::chrono::steady_clock::now();
        }

        if(IsHotkeyStateReqFulfilled(&Hotkey))
        {
            if(Hotkey.Command.empty())
                return true;

            if(Hotkey.IsSystemCommand)
                KwmExecuteThreadedSystemCommand(Hotkey.Command);
            else
                KwmInterpretCommand(Hotkey.Command, 0);

            return true;
        }
    }

    return false;
}

bool HotkeyExists(modifiers Mod, CGKeyCode Keycode, hotkey *Hotkey)
{
    hotkey TempHotkey;
    TempHotkey.Mod = Mod;
    TempHotkey.Key = Keycode;

    for(std::size_t HotkeyIndex = 0; HotkeyIndex < KWMHotkeys.List.size(); ++HotkeyIndex)
    {
        if(HotkeysAreEqual(&KWMHotkeys.List[HotkeyIndex], &TempHotkey))
        {
            if(Hotkey)
                *Hotkey = KWMHotkeys.List[HotkeyIndex];

            return true;
        }
    }

    return false;
}

void DetermineHotkeyState(hotkey *Hotkey, std::string &Command)
{
    std::size_t StartOfList = Command.find("{");
    std::size_t EndOfList = Command.find("}");

    bool Valid = !Command.empty() &&
                 StartOfList != std::string::npos &&
                 EndOfList != std::string::npos;

    if(Valid)
    {
        std::string Applications = Command.substr(StartOfList + 1, EndOfList - (StartOfList + 1));
        Hotkey->List = SplitString(Applications, ',');

        if(Command[Command.size()-2] == '-')
        {
            if(Command[Command.size()-1] == 'e')
                Hotkey->State = HotkeyStateExclude;
            else if(Command[Command.size()-1] == 'i')
                Hotkey->State = HotkeyStateInclude;
            else
                Hotkey->State = HotkeyStateNone;

            Command = Command.substr(0, StartOfList - 1);
        }
    }

    if(!Valid)
        Hotkey->State = HotkeyStateNone;
}

bool KwmParseHotkey(std::string KeySym, std::string Command, hotkey *Hotkey)
{
    std::vector<std::string> KeyTokens = SplitString(KeySym, '-');
    if(KeyTokens.size() != 2)
        return false;

    std::vector<std::string> Modifiers = SplitString(KeyTokens[0], '+');
    for(std::size_t ModIndex = 0; ModIndex < Modifiers.size(); ++ModIndex)
    {
        if(Modifiers[ModIndex] == "cmd")
            Hotkey->Mod.CmdKey = true;
        else if(Modifiers[ModIndex] == "alt")
            Hotkey->Mod.AltKey = true;
        else if(Modifiers[ModIndex] == "ctrl")
            Hotkey->Mod.CtrlKey = true;
        else if(Modifiers[ModIndex] == "shift")
            Hotkey->Mod.ShiftKey = true;
        else if(Modifiers[ModIndex] == "prefix")
            Hotkey->Prefixed = true;
    }

    DetermineHotkeyState(Hotkey, Command);
    Hotkey->IsSystemCommand = IsPrefixOfString(Command, "sys");
    Hotkey->Command = Command;

    CGKeyCode Keycode;
    bool Result = GetLayoutIndependentKeycode(KeyTokens[1], &Keycode);
    if(!Result)
        Result = KeycodeForChar(KeyTokens[1][0], &Keycode);

    Hotkey->Key = Keycode;
    return Result;
}

void KwmSetSpacesKey(std::string KeySym)
{
    modifiers Mod = {};
    std::vector<std::string> Modifiers = SplitString(KeySym, '+');
    for(std::size_t ModIndex = 0; ModIndex < Modifiers.size(); ++ModIndex)
    {
        if(Modifiers[ModIndex] == "cmd")
            Mod.CmdKey = true;
        else if(Modifiers[ModIndex] == "alt")
            Mod.AltKey = true;
        else if(Modifiers[ModIndex] == "ctrl")
            Mod.CtrlKey = true;
        else if(Modifiers[ModIndex] == "shift")
            Mod.ShiftKey = true;
    }
    KWMHotkeys.SpacesKey = Mod;
}

void KwmSetPrefix(std::string KeySym)
{
    hotkey Hotkey = {};
    if(KwmParseHotkey(KeySym, "", &Hotkey))
    {
        KWMHotkeys.Prefix.Key = Hotkey;
        KWMHotkeys.Prefix.Active = false;
        KWMHotkeys.Prefix.Enabled = true;
    }
}

void KwmSetPrefixGlobal(bool Global)
{
    KWMHotkeys.Prefix.Global = Global;
}

void KwmSetPrefixTimeout(double Timeout)
{
    KWMHotkeys.Prefix.Timeout = Timeout;
}

void KwmAddHotkey(std::string KeySym, std::string Command)
{
    hotkey Hotkey = {};
    if(KwmParseHotkey(KeySym, Command, &Hotkey) &&
       !HotkeyExists(Hotkey.Mod, Hotkey.Key, NULL))
            KWMHotkeys.List.push_back(Hotkey);
}

void KwmRemoveHotkey(std::string KeySym)
{
    hotkey NewHotkey = {};
    if(KwmParseHotkey(KeySym, "", &NewHotkey))
    {
        for(std::size_t HotkeyIndex = 0; HotkeyIndex < KWMHotkeys.List.size(); ++HotkeyIndex)
        {
            if(HotkeysAreEqual(&KWMHotkeys.List[HotkeyIndex], &NewHotkey))
            {
                KWMHotkeys.List.erase(KWMHotkeys.List.begin() + HotkeyIndex);
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
    else if(Key == "left")
        *Keycode =  0x7B;
    else if(Key == "right")
        *Keycode =  0x7C;
    else if(Key == "up")
        *Keycode = 0x7E;
    else if(Key == "down")
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

        for(std::size_t KeyIndex = 0; KeyIndex < 128; ++KeyIndex)
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
    for(std::size_t CharIndex = 0; CharIndex < Text.size(); ++CharIndex)
    {
        CFStringGetCharacters(TextRef, CFRangeMake(CharIndex, 1), &OutputBuffer);

        CGEventSetFlags(EventKeyDown, 0);
        CGEventKeyboardSetUnicodeString(EventKeyDown, 1, &OutputBuffer);
        CGEventPost(kCGHIDEventTap, EventKeyDown);

        CGEventSetFlags(EventKeyUp, 0);
        CGEventKeyboardSetUnicodeString(EventKeyUp, 1, &OutputBuffer);
        CGEventPost(kCGHIDEventTap, EventKeyUp);
    }

    CFRelease(EventKeyUp);
    CFRelease(EventKeyDown);
    CFRelease(TextRef);
}

void KwmEmitKeystroke(std::string KeySym)
{
    std::vector<std::string> KeyTokens = SplitString(KeySym, '-');
    if(KeyTokens.size() != 2)
        return;

    modifiers Mod = {};
    std::vector<std::string> Modifiers = SplitString(KeyTokens[0], '+');
    for(std::size_t ModIndex = 0; ModIndex < Modifiers.size(); ++ModIndex)
    {
        if(Modifiers[ModIndex] == "cmd")
            Mod.CmdKey = true;
        else if(Modifiers[ModIndex] == "alt")
            Mod.AltKey = true;
        else if(Modifiers[ModIndex] == "ctrl")
            Mod.CtrlKey = true;
        else if(Modifiers[ModIndex] == "shift")
            Mod.ShiftKey = true;
    }

    KwmEmitKeystroke(Mod, KeyTokens[1]);
}

void KwmEmitKeystroke(modifiers Mod, std::string Key)
{
    CGEventFlags Flags = 0;
    if(Mod.CmdKey)
        Flags |= kCGEventFlagMaskCommand;
    if(Mod.CtrlKey)
        Flags |= kCGEventFlagMaskControl;
    if(Mod.AltKey)
        Flags |= kCGEventFlagMaskAlternate;
    if(Mod.ShiftKey)
        Flags |= kCGEventFlagMaskShift;

    CGKeyCode Keycode;
    bool Result = GetLayoutIndependentKeycode(Key, &Keycode);
    if(!Result)
        Result = KeycodeForChar(Key[0], &Keycode);

    if(Result)
    {
        CGEventRef EventKeyDown = CGEventCreateKeyboardEvent(NULL, Keycode, true);
        CGEventRef EventKeyUp = CGEventCreateKeyboardEvent(NULL, Keycode, false);
        CGEventSetFlags(EventKeyDown, Flags);
        CGEventSetFlags(EventKeyUp, Flags);

        CGEventPost(kCGHIDEventTap, EventKeyDown);
        CGEventPost(kCGHIDEventTap, EventKeyUp);

        CFRelease(EventKeyDown);
        CFRelease(EventKeyUp);
    }
}
