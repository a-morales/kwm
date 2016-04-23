#ifndef KEYS_H
#define KEYS_H

#include "types.h"

bool HotkeysAreEqual(hotkey *A, hotkey *B);
bool KwmIsPrefixKey(hotkey *PrefixKey, modifiers *Mod, CGKeyCode Keycode);
bool HotkeyExists(modifiers Mod, CGKeyCode Keycode, hotkey *Hotkey, std::string Mode);
void DetermineHotkeyState(hotkey *Hotkey, std::string &Command);
bool IsHotkeyStateReqFulfilled(hotkey *Hotkey);

bool ShouldKeyBeProcessed(hotkey *Hotkey);
void CreateHotkeyFromCGEvent(CGEventRef Event, hotkey *Hotkey);
bool KwmParseHotkey(std::string KeySym, std::string Command, hotkey *Hotkey, bool Passthrough);
void KwmAddHotkey(std::string KeySym, std::string Command, bool Passthrough);
void KwmRemoveHotkey(std::string KeySym);
void KwmExecuteHotkey(hotkey *Hotkey);
void *KwmMainHotkeyTrigger(void *EventPtr);
void KwmEmitKeystrokes(std::string Text);
void KwmEmitKeystroke(modifiers Mod, std::string Key);
void KwmEmitKeystroke(std::string KeySym);

mode *GetBindingMode(std::string Mode);
void KwmActivateBindingMode(std::string Mode);
void KwmSetSpacesKey(std::string KeySym);
void CheckPrefixTimeout();

CFStringRef KeycodeToString(CGKeyCode Keycode);
bool KeycodeForChar(char Key, CGKeyCode *Keycode);
bool GetLayoutIndependentKeycode(std::string Key, CGKeyCode *Keycode);

#endif
