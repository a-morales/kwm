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
    std::string Name;
    int PID, WID;
    int X, Y;
    int Width, Height;
};

struct window_layout
{
    std::string Name;
    int X, Y;
    int Width, Height;
    int GapX, GapY;
    int GapVertical, GapHorizontal;
};

struct window_group_layout
{
    std::string Name;
    std::vector<int> TileWID;
    std::vector<window_layout> Layouts;
};

struct screen_info
{
    int ID;
    int X, Y;
    int Width, Height;
};

struct screen_layout
{
    int NumberOfLayouts;
    std::vector<window_group_layout> Layouts;    
};

struct spaces_info
{
    int ActiveLayoutIndex;
    int NextLayoutIndex;
    std::vector<int> Windows;
};

bool CheckPrivileges();

void GetActiveDisplays();
screen_info *GetDisplayOfWindow(window_info *Window);
std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex);

int GetSpaceOfWindow(window_info *Window);
void GetSpacesInfo(const void *Key, const void *Value, void *Context);
void RefreshActiveSpacesInfo();
void GetActiveSpaces();

void ToggleFocusedWindowFullscreen();
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
void ApplyLayoutForWindow(AXUIElementRef WindowRef, window_info *Window, window_layout *Layout);
int GetLayoutIndexOfWindow(window_info *Window);

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
