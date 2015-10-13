#ifndef KWM_H
#define KWM_H

#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <string>

#include <string.h>
#include <stdlib.h>

struct window_layout;

struct window_info
{
    std::string name;
    int pid, wid;
    int x, y;
    int width, height;
    window_layout *layout;
    int layout_index;
};

struct window_layout
{
    std::string name;
    int x, y;
    int width, height;
    int gap_x, gap_y;
    int gap_vertical, gap_horizontal;
};

struct window_group_layout
{
    std::string name;
    std::vector<window_layout> layouts;
};

struct screen_info
{
    int id;
    int x, y;
    int width, height;
};

struct screen_layout
{
    int number_of_layouts;
    std::vector<window_group_layout> layouts;    
};

struct spaces_info
{
    int active_layout_index;
    int next_layout_index;
    std::vector<int> windows;
};

bool CheckPrivileges();

void GetActiveDisplays();
screen_info *GetDisplayOfWindow(window_info *window);
std::vector<window_info*> GetAllWindowsOnDisplay(int screen_index);

int GetSpaceOfWindow(window_info *window);
void GetSpacesInfo(const void *key, const void *value, void *context);
void RefreshActiveSpacesInfo();
void GetActiveSpaces();

void ApplyLayoutForDisplay(int screen_index);
void CycleFocusedWindowLayout(int screen_index, int shift);
void CycleWindowInsideLayout(int screen_index);
void CycleFocusedWindowDisplay(int shift);

bool GetExpressionFromShiftDirection(window_info *window, const std::string &direction);
void ShiftWindowFocus(const std::string &direction);

void InitWindowLayouts();
void SetWindowLayoutValues(window_layout *layout, int x, int y, int width, int height);
window_layout GetWindowLayoutForScreen(int screen_index, const std::string &name);

void SetWindowDimensions(AXUIElementRef app_window, window_info *window, int x, int y, int width, int height);
window_layout *GetLayoutOfWindow(window_info *window);
bool WindowHasLayout(window_info *window, window_layout *layout);

bool KwmHotkeyCommands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
bool SystemHotkeyPassthrough(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);
bool CustomHotkeyCommands(bool cmd_key, bool ctrl_key, bool alt_key, CGKeyCode keycode);

bool IsWindowBelowCursor(window_info *window);
void DetectWindowBelowCursor();

bool GetWindowRef(window_info *window, AXUIElementRef *window_ref);
void GetWindowInfo(const void *key, const void *value, void *context);
window_info GetWindowInfoFromRef(AXUIElementRef window_ref);
bool WindowsAreEqual(window_info *window, window_info *match);

CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
void Fatal(const std::string &err);

#endif
