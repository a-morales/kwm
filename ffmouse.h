#ifndef FFMOUSE_H
#define FFMOUSE_H

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <string>

#include <string.h>
#include <stdlib.h>

struct app_info
{
    std::string owner;
    std::string name;
    int layer;
    int pid;
    int x,y;
    int width, height;
};

bool toggle_tap_hotkey(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
bool system_hotkey_passthrough(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
bool custom_hotkey_commands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
CGEventRef cgevent_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
void detect_window_below_cursor();
void get_window_info(const void *key, const void *value, void *context);
void fatal(const std::string &err);

#endif
