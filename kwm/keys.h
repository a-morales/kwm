#ifndef KEYS_H
#define KEYS_H

#include "types.h"

bool HotkeysAreEqual(hotkey *A, hotkey *B);
bool KwmIsPrefixKey(hotkey *PrefixKey, modifiers *Mod, CGKeyCode Keycode);
bool HotkeyExists(modifiers Mod, CGKeyCode Keycode, hotkey *Hotkey);
void DetermineHotkeyState(hotkey *Hotkey, std::string &Command);

bool KwmParseHotkey(std::string KeySym, std::string Command, hotkey *Hotkey);
void KwmAddHotkey(std::string KeySym, std::string Command);
void KwmRemoveHotkey(std::string KeySym);
bool KwmExecuteHotkey(modifiers Mod, CGKeyCode Keycode);
bool KwmMainHotkeyTrigger(CGEventRef *Event);
void KwmEmitKeystrokes(std::string Text);

void KwmSetPrefix(std::string KeySym);
void KwmSetPrefixGlobal(bool Global);
void KwmSetPrefixTimeout(double Timeout);

CFStringRef KeycodeToString(CGKeyCode Keycode);
bool KeycodeForChar(char Key, CGKeyCode *Keycode);
bool GetLayoutIndependentKeycode(std::string Key, CGKeyCode *Keycode);

#endif
