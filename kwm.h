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
    std::vector<int> FloatingWindowLst;

    AXUIElementRef FocusedWindowRef;
    window_info *FocusedWindow;
    focus_option KwmFocusMode;
    int KwmSplitMode;

    void (*DetectWindowBelowCursor)();
    void (*SetWindowDimensions)(AXUIElementRef, window_info *, int, int, int, int);
    void (*ToggleFocusedWindowFullscreen)();
    void (*SwapFocusedWindowWithNearest)(int);
    void (*ShiftWindowFocus)(int);
    void (*CycleFocusedWindowDisplay)(int);
    void (*AddWindowToTree)();
    void (*RemoveWindowFromTree)();
    void (*KwmRestart)();
};

struct node_container
{    
    double X,Y;
    double Width, Height;
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
    std::string Owner;
    int PID, WID;
    int Layer;
    int X, Y;
    int Width, Height;
};

struct screen_info
{
    int ID;
    int X, Y;
    double Width, Height;
    double PaddingTop, PaddingBottom;
    double PaddingLeft, PaddingRight;
    std::vector<tree_node*> Space;
};

tree_node *CreateTreeFromWindowIDList(screen_info *, std::vector<int>);
tree_node *CreateRootNode();
tree_node *CreateLeafNode(screen_info *, tree_node *, int, int);
tree_node *GetNearestNodeToTheLeft(tree_node *);
tree_node *GetNearestNodeToTheRight(tree_node *);
tree_node *GetNodeFromWindowID(tree_node *, int);
bool IsLeafNode(tree_node *);
void CreateLeafNodePair(screen_info *, tree_node *, int, int, int);
void SwapNodeWindowIDs(tree_node *, tree_node *);
void SetRootNodeContainer(tree_node *, int, int, int, int);
void ApplyNodeContainer(tree_node *);
void AddWindowToTree(int WindowID);
void AddWindowToTree();
void RemoveWindowFromTree();
void DestroyNodeTree(tree_node *);
node_container LeftVerticalContainerSplit(screen_info *, tree_node *);
node_container RightVerticalContainerSplit(screen_info *, tree_node *);
node_container UpperHorizontalContainerSplit(screen_info *, tree_node *);
node_container LowerHorizontalContainerSplit(screen_info *, tree_node *);

void GetActiveDisplays();
screen_info *GetDisplayOfWindow(window_info *);
std::vector<window_info*> GetAllWindowsOnDisplay(int);
std::vector<int> GetAllWindowIDsOnDisplay(int);
void CycleFocusedWindowDisplay(int);

void ShouldWindowNodeTreeUpdate(int);
void CreateWindowNodeTree();
void RefreshWindowNodeTree();
void ResizeWindow(tree_node *);
void SetWindowDimensions(AXUIElementRef, window_info *, int, int, int, int);

int NumberOfSpaces();
void AddWindowToSpace(int, int);
int GetSpaceOfWindow(window_info *);
void GetActiveSpaces();
void GetSpaceInfo(const void *, const void *, void *);

void ToggleFocusedWindowFullscreen();
void SwapFocusedWindowWithNearest(int);
void ShiftWindowFocus(int);
bool WindowsAreEqual(window_info *, window_info *);
bool FilterWindowList();
bool IsWindowBelowCursor(window_info *);
void DetectWindowBelowCursor();
void CheckIfSpaceTransitionOccurred();
bool IsWindowFloating(int);

void CloseWindowByRef(AXUIElementRef);
void CloseWindow(window_info *);
void SetWindowRefFocus(AXUIElementRef, window_info *);
void SetWindowFocus(window_info *);
void WriteNameOfFocusedWindowToFile();

std::string GetWindowTitle(AXUIElementRef);
CGPoint GetWindowPos(AXUIElementRef);
CGSize GetWindowSize(AXUIElementRef);
window_info *GetWindowByID(int);
bool GetWindowRef(window_info *, AXUIElementRef *);
void GetWindowInfo(const void *, const void *, void *);

kwm_code LoadKwmCode();
void UnloadKwmCode(kwm_code *);
void BuildExportTable();
std::string KwmGetFileTime(const char *);

void KwmInit();
void KwmRestart();
bool CheckPrivileges();
void Fatal(const std::string &);
CGEventRef CGEventCallback(CGEventTapProxy, CGEventType, CGEventRef, void *);

#endif
