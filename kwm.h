#ifndef KWM_H
#define KWM_H

#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <map>
#include <string>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

struct kwm_code;
struct export_table;

struct window_info;
struct screen_info;
struct node_container;
struct tree_node;

#ifdef DEBUG_BUILD
    #define DEBUG(x) std::cout << x << std::endl;
#else
    #define DEBUG(x)
#endif

#define KWM_HOTKEY_COMMANDS(name) bool name(export_table *EX, bool CmdKey, bool CtrlKey, bool AltKey, CGKeyCode Keycode)
typedef KWM_HOTKEY_COMMANDS(kwm_hotkey_commands);

enum focus_option
{ 
    FocusFollowsMouse, 
    FocusAutoraise, 
    FocusDisabled 
};

struct kwm_code
{
    void *KwmHotkeySO;
    std::string HotkeySOFileTime;

    kwm_hotkey_commands *KWMHotkeyCommands;
    kwm_hotkey_commands *SystemHotkeyCommands;
    kwm_hotkey_commands *CustomHotkeyCommands;

    bool IsValid;
};

struct export_table
{
    std::string KwmFilePath;

    AXUIElementRef FocusedWindowRef;
    window_info *FocusedWindow;
    focus_option KwmFocusMode;

    void (*DetectWindowBelowCursor)();
    void (*SetWindowDimensions)(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height);
    void (*ToggleFocusedWindowFullscreen)();
    void (*SwapFocusedWindowWithNearest)(int Shift);
    void (*ShiftWindowFocus)(int Shift);
    void (*CycleFocusedWindowDisplay)(int Shift);
    void (*KwmRestart)();
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
    int Layer;
    int X, Y;
    int Width, Height;
};

struct screen_info
{
    int ID;
    int X, Y;
    int Width, Height;
    int PaddingTop, PaddingBottom;
    int PaddingLeft, PaddingRight;
    std::vector<tree_node*> Space;
};

void SwapNodeWindowIDs(tree_node *A, tree_node *B);
tree_node *GetNearestNodeToTheLeft(tree_node *Node);
tree_node *GetNearestNodeToTheRight(tree_node *Node);
tree_node *GetNodeFromWindowID(tree_node *Node, int WindowID);
tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<int> Windows);
void CreateLeafNodePair(screen_info *Screen, tree_node *Parent, int LeftWindowID, int RightWindowID, int SplitMode);
void SetRootNodeContainer(tree_node *Node, int X, int Y, int Width, int Height);
void ApplyNodeContainer(tree_node *Node);
void DestroyNodeTree(tree_node *Node);
tree_node *CreateLeafNode(screen_info *Screen, tree_node *Parent, int WindowID, int ContainerType);
tree_node *CreateRootNode();
node_container FullscreenContainer(screen_info *Screen, tree_node *Node);
node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node);
node_container UpperHorizontalContainerSplit(screen_info *Screen, tree_node *Node);
node_container LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node);
node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node);

void GetActiveDisplays();
screen_info *GetDisplayOfWindow(window_info *Window);
std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex);
std::vector<int> GetAllWindowIDsOnDisplay(int ScreenIndex);
void CycleFocusedWindowDisplay(int Shift);

void ShouldWindowNodeTreeUpdate()
void CreateWindowNodeTree();
void RefreshWindowNodeTree();
void ResizeWindow(tree_node *Node);
void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height);

int NumberOfSpaces();
void AddWindowToSpace(int WindowID, int SpaceIndex);
int GetSpaceOfWindow(window_info *Window);
void GetActiveSpaces();
void GetSpaceInfo(const void *Key, const void *Value, void *Context);

void ToggleFocusedWindowFullscreen();
void SwapFocusedWindowWithNearest(int Shift);
void ShiftWindowFocus(int Shift);
bool WindowsAreEqual(window_info *Window, window_info *Match);
void FilterWindowList();
bool IsWindowBelowCursor(window_info *Window);
void DetectWindowBelowCursor();
void CheckIfSpaceTransitionOccurred()

void CloseWindowByRef(AXUIElementRef WindowRef);
void CloseWindow(window_info *Window);
void SetWindowRefFocus(AXUIElementRef WindowRef, window_info *Window);
void SetWindowFocus(window_info *Window);

std::string GetWindowTitle(AXUIElementRef WindowRef);
CGPoint GetWindowPos(AXUIElementRef WindowRef);
CGSize GetWindowSize(AXUIElementRef WindowRef);
window_info *GetWindowByID(int WindowID);
bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef);
void GetWindowInfo(const void *Key, const void *Value, void *Context);

kwm_code LoadKwmCode();
void UnloadKwmCode(kwm_code *Code);
void BuildExportTable();
std::string KwmGetFileTime(const char *File);

void KwmInit();
void KwmRestart();
bool CheckPrivileges();
void Fatal(const std::string &Err);
CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon);

#endif
