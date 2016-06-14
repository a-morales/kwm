#include "keys.h"
#include "kwm.h"
#include "helpers.h"
#include "interpreter.h"
#include "border.h"
#include "command.h"

#include "axlib/event.h"

#define internal static

extern kwm_focus KWMFocus;
extern kwm_hotkeys KWMHotkeys;
extern kwm_thread KWMThread;

internal CFStringRef
KeycodeToString(CGKeyCode Keycode)
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

internal bool
KeycodeForChar(char Key, CGKeyCode *Keycode)
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

internal bool
GetLayoutIndependentKeycode(std::string Key, CGKeyCode *Keycode)
{
    bool Result = true;

    if(Key == "return")
        *Keycode = kVK_Return;
    else if(Key == "tab")
        *Keycode = kVK_Tab;
    else if(Key == "space")
        *Keycode = kVK_Space;
    else if(Key == "backspace")
        *Keycode = kVK_Delete;
    else if(Key == "delete")
        *Keycode = kVK_ForwardDelete;
    else if(Key == "escape")
        *Keycode =  kVK_Escape;
    else if(Key == "left")
        *Keycode =  kVK_LeftArrow;
    else if(Key == "right")
        *Keycode =  kVK_RightArrow;
    else if(Key == "up")
        *Keycode = kVK_UpArrow;
    else if(Key == "down")
        *Keycode = kVK_DownArrow;
    else if(Key == "f1")
        *Keycode = kVK_F1;
    else if(Key == "f2")
        *Keycode = kVK_F2;
    else if(Key == "f3")
        *Keycode = kVK_F3;
    else if(Key == "f4")
        *Keycode = kVK_F4;
    else if(Key == "f5")
        *Keycode = kVK_F5;
    else if(Key == "f6")
        *Keycode = kVK_F6;
    else if(Key == "f7")
        *Keycode = kVK_F7;
    else if(Key == "f8")
        *Keycode = kVK_F8;
    else if(Key == "f9")
        *Keycode = kVK_F9;
    else if(Key == "f10")
        *Keycode = kVK_F10;
    else if(Key == "f11")
        *Keycode = kVK_F11;
    else if(Key == "f12")
        *Keycode = kVK_F12;
    else if(Key == "f13")
        *Keycode = kVK_F13;
    else if(Key == "f14")
        *Keycode = kVK_F14;
    else if(Key == "f15")
        *Keycode = kVK_F15;
    else if(Key == "f16")
        *Keycode = kVK_F16;
    else if(Key == "f17")
        *Keycode = kVK_F17;
    else if(Key == "f18")
        *Keycode = kVK_F18;
    else if(Key == "f19")
        *Keycode = kVK_F19;
    else if(Key == "f20")
        *Keycode = kVK_F19;
    else
        Result = false;

    return Result;
}

internal bool
DoesBindingModeExist(std::string Mode)
{
    std::map<std::string, mode>::iterator It = KWMHotkeys.Modes.find(Mode);
    return !Mode.empty() && It != KWMHotkeys.Modes.end();
}


internal bool
HotkeysAreEqual(hotkey *A, hotkey *B)
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

internal bool
IsHotkeyStateReqFulfilled(hotkey *Hotkey)
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

internal void
KwmExecuteHotkey(hotkey *Hotkey)
{
    if(Hotkey->Command.empty())
        return;

    std::vector<std::string> Commands = SplitString(Hotkey->Command, ';');
    DEBUG("KwmExecuteHotkey: Number of commands " << Commands.size());
    for(int CmdIndex = 0; CmdIndex < Commands.size(); ++CmdIndex)
    {
        std::string &Command = TrimString(Commands[CmdIndex]);
        if(!Command.empty())
        {
            DEBUG("KwmExecuteHotkey() " << Command);
            if(IsPrefixOfString(Command, "exec"))
                KwmExecuteThreadedSystemCommand(Command);
            else
                KwmInterpretCommand(Command, 0);

            if(KWMHotkeys.ActiveMode->Prefix)
                KWMHotkeys.ActiveMode->Time = std::chrono::steady_clock::now();
        }
    }
}

internal void
DetermineHotkeyState(hotkey *Hotkey, std::string &Command)
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

internal bool
KwmParseHotkey(std::string KeySym, std::string Command, hotkey *Hotkey, bool Passthrough, bool KeycodeInHex)
{
    std::vector<std::string> KeyTokens = SplitString(KeySym, '-');
    if(KeyTokens.size() != 2)
        return false;

    Hotkey->Mode = "default";
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
        else
            Hotkey->Mode = Modifiers[ModIndex];
    }

    DetermineHotkeyState(Hotkey, Command);
    Hotkey->Passthrough = Passthrough;
    Hotkey->Command = Command;

    CGKeyCode Keycode;
    bool Result = false;
    if(KeycodeInHex)
    {
        Result = true;
        Keycode = ConvertHexStringToInt(KeyTokens[1]);
        DEBUG("bindcode: " << Keycode);
    }
    else
    {
        Result = GetLayoutIndependentKeycode(KeyTokens[1], &Keycode);
        if(!Result)
            Result = KeycodeForChar(KeyTokens[1][0], &Keycode);
    }

    Hotkey->Key = Keycode;
    return Result;
}

void KwmAddHotkey(std::string KeySym, std::string Command, bool Passthrough, bool KeycodeInHex)
{
    hotkey Hotkey = {};
    if(KwmParseHotkey(KeySym, Command, &Hotkey, Passthrough, KeycodeInHex) &&
       !HotkeyExists(Hotkey.Mod, Hotkey.Key, NULL, Hotkey.Mode))
        KWMHotkeys.Modes[Hotkey.Mode].Hotkeys.push_back(Hotkey);
}

void KwmRemoveHotkey(std::string KeySym, bool KeycodeInHex)
{
    hotkey NewHotkey = {};
    if(KwmParseHotkey(KeySym, "", &NewHotkey, false, KeycodeInHex))
    {
        mode *BindingMode = GetBindingMode(NewHotkey.Mode);
        for(std::size_t HotkeyIndex = 0; HotkeyIndex < BindingMode->Hotkeys.size(); ++HotkeyIndex)
        {
            hotkey *CurrentHotkey = &BindingMode->Hotkeys[HotkeyIndex];
            if(HotkeysAreEqual(CurrentHotkey, &NewHotkey))
            {
                BindingMode->Hotkeys.erase(BindingMode->Hotkeys.begin() + HotkeyIndex);
                break;
            }
        }
    }
}

mode *GetBindingMode(std::string Mode)
{
    std::map<std::string, mode>::iterator It = KWMHotkeys.Modes.find(Mode);
    if(It == KWMHotkeys.Modes.end())
    {
        mode NewMode = {};
        NewMode.Name = Mode;
        KWMHotkeys.Modes[Mode] = NewMode;
    }

    return &KWMHotkeys.Modes[Mode];
}

void KwmActivateBindingMode(std::string Mode)
{
    mode *BindingMode = GetBindingMode(Mode);
    if(!DoesBindingModeExist(Mode))
        BindingMode = GetBindingMode("default");

    KWMHotkeys.ActiveMode = BindingMode;
    UpdateBorder("focused");
    if(BindingMode->Prefix)
        BindingMode->Time = std::chrono::steady_clock::now();
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

void CheckPrefixTimeout()
{
    if(KWMHotkeys.ActiveMode->Prefix)
    {
        kwm_time_point NewPrefixTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> Diff = NewPrefixTime - KWMHotkeys.ActiveMode->Time;
        if(Diff.count() > KWMHotkeys.ActiveMode->Timeout)
        {
            DEBUG("Prefix timeout expired. Switching to mode " << KWMHotkeys.ActiveMode->Restore);
            KwmActivateBindingMode(KWMHotkeys.ActiveMode->Restore);
        }
    }
}

void CreateHotkeyFromCGEvent(CGEventRef Event, hotkey *Hotkey)
{
    CGEventFlags Flags = CGEventGetFlags(Event);
    Hotkey->Mod.CmdKey = (Flags & kCGEventFlagMaskCommand) == kCGEventFlagMaskCommand;
    Hotkey->Mod.AltKey = (Flags & kCGEventFlagMaskAlternate) == kCGEventFlagMaskAlternate;
    Hotkey->Mod.CtrlKey = (Flags & kCGEventFlagMaskControl) == kCGEventFlagMaskControl;
    Hotkey->Mod.ShiftKey = (Flags & kCGEventFlagMaskShift) == kCGEventFlagMaskShift;
    Hotkey->Key = (CGKeyCode)CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);
}

bool HotkeyExists(modifiers Mod, CGKeyCode Keycode, hotkey *Hotkey, std::string Mode)
{
    hotkey TempHotkey = {};
    TempHotkey.Mod = Mod;
    TempHotkey.Key = Keycode;

    mode *BindingMode = GetBindingMode(Mode);
    for(std::size_t HotkeyIndex = 0; HotkeyIndex < BindingMode->Hotkeys.size(); ++HotkeyIndex)
    {
        hotkey *CheckHotkey = &BindingMode->Hotkeys[HotkeyIndex];
        if(HotkeysAreEqual(CheckHotkey, &TempHotkey))
        {
            if(Hotkey)
                *Hotkey = *CheckHotkey;

            return true;
        }
    }

    return false;
}

EVENT_CALLBACK(Callback_AXEvent_HotkeyPressed)
{
    hotkey *Hotkey = (hotkey *) Event->Context;
    DEBUG("AXEvent_HotkeyPressed: Hotkey activated");

    if(IsHotkeyStateReqFulfilled(Hotkey))
        KwmExecuteHotkey(Hotkey);

    free(Hotkey);
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
