#ifndef FFMOUSE_H
#define FFMOUSE_H

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
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

struct screen_info
{
    int id;
    int x, y;
    int width, height;
};

struct window_layout
{
    std::string name;
    int x, y;
    int gap_x, gap_y;
    int gap_vertical, gap_horizontal;
    int width, height;
};

bool check_privileges();
void request_privileges();

void get_active_displays();
screen_info *get_display_of_window();

void init_window_layouts();
void set_window_layout_values(window_layout *layout, int x, int y, int width, int height);
window_layout get_window_layout_for_screen(const std::string &name);
void set_window_dimensions(int x, int y, int width, int height);
void send_window_to_prev_screen();
void send_window_to_next_screen();

bool toggle_tap_hotkey(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
bool system_hotkey_passthrough(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
bool custom_hotkey_commands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
CGEventRef cgevent_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
void detect_window_below_cursor();
void get_window_info(const void *key, const void *value, void *context);
void fatal(const std::string &err);

#endif
