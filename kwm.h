#ifndef KWM_H
#define KWM_H

#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <string>

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_BUILD
    #define DEBUG(x) std::cout << x << std::endl;
#else
    #define DEBUG(x)
#endif

struct window_info;
struct screen_info;
struct spaces_info;
struct node_container;
struct tree_node;

enum focus_option
{ 
    FocusFollowsMouse, 
    FocusAutoraise, 
    FocusDisabled 
};

struct node_container
{    
    int X,Y;
    int Width, Height;
};

struct tree_node
{
    int WindowID;
    node_container Container;
    tree_node *Parent;
    tree_node *LeftChild;
    tree_node *RightChild;
};

struct window_info
{
    std::string Name;
    int PID, WID;
    int X, Y;
    int Width, Height;
};

struct spaces_info
{
    std::vector<int> Windows;
    tree_node *RootNode;
};

struct screen_info
{
    int ID;
    int X, Y;
    int Width, Height;
    int PaddingTop, PaddingBottom;
    int PaddingLeft, PaddingRight;
};

tree_node *CreateNode(int WindowID, node_container *Container,
                      tree_node *Parent, tree_node *LeftChild, tree_node *RightChild);

tree_node *CreateNode(int WindowID, int X, int Y, int Width, int Height,
                      tree_node *Parent, tree_node *LeftChild, tree_node *RightChild);

tree_node *CreateTreeFromWindowIDList(std::vector<int> *Windows);
void DestroyNode(tree_node *Node);
void ApplyNodeContainer(tree_node *Node);

void GetActiveDisplays();
screen_info *GetDisplayOfWindow(window_info *Window);
std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex);
void CycleFocusedWindowDisplay(int Shift);

int GetSpaceOfWindow(window_info *Window);
void GetSpacesInfo(const void *Key, const void *Value, void *Context);
void RefreshActiveSpacesInfo();
void GetActiveSpaces();

void ResizeWindow(tree_node *Node);
void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, 
                         int X, int Y, int Width, int Height);

bool KwmHotkeyCommands(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode);
bool SystemHotkeyPassthrough(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode);
bool CustomHotkeyCommands(bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode);

bool WindowsAreEqual(window_info *Window, window_info *Match);
bool IsWindowBelowCursor(window_info *Window);
void DetectWindowBelowCursor();

void CloseWindowByRef(AXUIElementRef WindowRef);
void CloseFocusedWindow();
void CloseWindow(window_info *Window);
void SetWindowRefFocus(AXUIElementRef WindowRef, window_info *Window);
void SetWindowFocus(window_info *Window);

std::string GetWindowTitle(AXUIElementRef WindowRef);
CGPoint GetWindowPos(AXUIElementRef WindowRef);
CGSize GetWindowSize(AXUIElementRef WindowRef);
bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef);
void GetWindowInfo(const void *Key, const void *Value, void *Context);

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon);
bool CheckPrivileges();
void Fatal(const std::string &Err);

#endif
