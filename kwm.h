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
screen_info *GetDisplayOfWindow(window_info *Window);
std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex);

int GetSpaceOfWindow(window_info *Window);
void GetSpacesInfo(const void *Key, const void *Value, void *Context);
void RefreshActiveSpacesInfo();
void GetActiveSpaces();

void ApplyLayoutForDisplay(int ScreenIndex);
void CycleFocusedWindowLayout(int ScreenIndex, int Shift);
void CycleWindowInsideLayout(int ScreenIndex);
void CycleFocusedWindowDisplay(int Shift);

bool GetExpressionFromShiftDirection(window_info *Window, const std::string &Direction);
void ShiftWindowFocus(const std::string &Direction);

void InitWindowLayouts();
void SetWindowLayoutValues(window_layout *Layout, int X, int Y, int Width, int Height);
window_layout GetWindowLayoutForScreen(int ScreenIndex, const std::string &Name);

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height);
window_layout *GetLayoutOfWindow(window_info *Window);
bool WindowHasLayout(window_info *Window, window_layout *Kayout);

bool KwmHotkeyCommands(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode);
bool SystemHotkeyPassthrough(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode);
bool CustomHotkeyCommands(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode);

bool IsWindowBelowCursor(window_info *Window);
void DetectWindowBelowCursor();

bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef);
void GetWindowInfo(const void *Key, const void *Value, void *Context);
window_info GetWindowInfoFromRef(AXUIElementRef WindowRef);
bool WindowsAreEqual(window_info *Window, window_info *Match);

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon);
void Fatal(const std::string &Err);

#endif
