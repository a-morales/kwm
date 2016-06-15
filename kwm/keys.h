#ifndef KEYS_H
#define KEYS_H

#include "types.h"

void CreateHotkeyFromCGEvent(CGEventRef Event, hotkey *Hotkey);

void KwmAddHotkey(std::string KeySym, std::string Command, bool Passthrough, bool KeycodeInHex);
void KwmRemoveHotkey(std::string KeySym, bool KeycodeInHex);
bool HotkeyExists(modifiers Mod, CGKeyCode Keycode, hotkey *Hotkey, std::string &Mode);
void KwmEmitKeystrokes(std::string Text);
void KwmEmitKeystroke(modifiers Mod, std::string Key);
void KwmEmitKeystroke(std::string KeySym);

mode *GetBindingMode(std::string Mode);
void KwmActivateBindingMode(std::string Mode);
void CheckPrefixTimeout();

#endif
