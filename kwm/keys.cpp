#include "kwm.h"

extern ProcessSerialNumber FocusedPSN;
extern std::vector<hotkey> KwmHotkeys;

bool KwmExecuteHotkey(bool CmdKey, bool CtrlKey, bool AltKey, bool ShiftKey, CGKeyCode Keycode)
{
    hotkey Hotkey = {};
    if(HotkeyExists(CmdKey, CtrlKey, AltKey, ShiftKey, Keycode, &Hotkey))
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

bool HotkeyExists(bool CmdKey, bool CtrlKey, bool AltKey, bool ShiftKey, CGKeyCode Keycode, hotkey *Hotkey)
{
    for(int HotkeyIndex = 0; HotkeyIndex < KwmHotkeys.size(); ++HotkeyIndex)
    {
        hotkey *CurrentHotkey = &KwmHotkeys[HotkeyIndex];
        if(CurrentHotkey->CmdKey == CmdKey &&
           CurrentHotkey->CtrlKey == CtrlKey &&
           CurrentHotkey->AltKey == AltKey &&
           CurrentHotkey->ShiftKey == ShiftKey &&
           CurrentHotkey->Key == Keycode)
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
            Hotkey->CmdKey = true;
        else if(Modifiers[ModIndex] == "alt")
            Hotkey->AltKey = true;
        else if(Modifiers[ModIndex] == "ctrl")
            Hotkey->CtrlKey = true;
        else if(Modifiers[ModIndex] == "shift")
            Hotkey->ShiftKey = true;
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

void KwmAddHotkey(std::string KeySym, std::string Command)
{
    hotkey Hotkey = {};
    if(KwmParseHotkey(KeySym, Command, &Hotkey) &&
       !HotkeyExists(Hotkey.CmdKey, Hotkey.CtrlKey, Hotkey.AltKey, Hotkey.ShiftKey, Hotkey.Key, NULL))
            KwmHotkeys.push_back(Hotkey);
}

void KwmRemoveHotkey(std::string KeySym)
{
    hotkey NewHotkey = {};
    if(KwmParseHotkey(KeySym, "", &NewHotkey))
    {
        for(int HotkeyIndex = 0; HotkeyIndex < KwmHotkeys.size(); ++HotkeyIndex)
        {
            hotkey *CurrentHotkey = &KwmHotkeys[HotkeyIndex];
            if(CurrentHotkey->CmdKey == NewHotkey.CmdKey &&
               CurrentHotkey->CtrlKey == NewHotkey.CtrlKey &&
               CurrentHotkey->AltKey == NewHotkey.AltKey &&
               CurrentHotkey->ShiftKey == NewHotkey.ShiftKey &&
               CurrentHotkey->Key == NewHotkey.Key)
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

CFStringRef KeycodeToString(CGKeyCode KeyCode)
{
    TISInputSourceRef Keyboard = TISCopyCurrentKeyboardInputSource();
    CFDataRef Uchr = (CFDataRef)TISGetInputSourceProperty(Keyboard, kTISPropertyUnicodeKeyLayoutData);
    const UCKeyboardLayout *KeyboardLayout = (const UCKeyboardLayout*)CFDataGetBytePtr(Uchr);

    if(KeyboardLayout)
    {
        UInt32 DeadKeyState = 0;
        UniCharCount MaxStringLength = 255;
        UniCharCount ActualStringLength = 0;
        UniChar UnicodeString[MaxStringLength];

        OSStatus Status = UCKeyTranslate(KeyboardLayout, KeyCode,
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
        CGEventPostToPSN(&FocusedPSN, EventKeyDown);

        CGEventSetFlags(EventKeyUp, 0);
        CGEventKeyboardSetUnicodeString(EventKeyUp, 1, &OutputBuffer);
        CGEventPostToPSN(&FocusedPSN, EventKeyUp);
    }

    CFRelease(EventKeyUp);
    CFRelease(EventKeyDown);
    CFRelease(TextRef);
}

