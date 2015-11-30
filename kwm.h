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

#include <pthread.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "CGSSpace.h"

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

#define KWM_KEY_REMAP(name) void name(CGEventRef Event, bool *CmdKey, bool *CtrlKey, bool *AltKey, CGKeyCode Keycode, int *Result)
typedef KWM_KEY_REMAP(kwm_key_remap);

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, int *);
extern "C" CGSConnectionID _CGSDefaultConnection(void);
#define CGSDefaultConnection _CGSDefaultConnection()

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
    kwm_key_remap *RemapKeys;

    bool IsValid;
};

struct export_table
{
    std::string KwmFilePath;
    focus_option KwmFocusMode;
    int KwmSplitMode;

    void (*ChangeGapOfDisplay)(const std::string &, int);
    void (*ChangePaddingOfDisplay)(const std::string &, int);
    void (*FocusWindowBelowCursor)();
    void (*ReflectWindowNodeTreeVertically)();
    void (*ResizeWindowToContainerSize)();
    void (*MoveContainerSplitter)(int, int);
    void (*ToggleFocusedWindowFullscreen)();
    void (*ToggleFocusedWindowParentContainer)();
    void (*SwapFocusedWindowWithNearest)(int);
    void (*ShiftWindowFocus)(int);
    void (*CycleFocusedWindowDisplay)(int);
    void (*ToggleFocusedWindowFloating)();
    void (*MarkWindowContainer)();
    void (*KwmRestart)();
};

struct node_container
{    
    double X, Y;
    double Width, Height;
    int Type;
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
    double VerticalGap, HorizontalGap;
    std::map<int, tree_node*> Space;
    int OldWindowListCount;
};

node_container LeftVerticalContainerSplit(screen_info *, tree_node *);
node_container RightVerticalContainerSplit(screen_info *, tree_node *);
node_container UpperHorizontalContainerSplit(screen_info *, tree_node *);
node_container LowerHorizontalContainerSplit(screen_info *, tree_node *);

void SetRootNodeContainer(screen_info *, tree_node *);
void CreateNodeContainer(screen_info *, tree_node *, int);
void CreateNodeContainerPair(screen_info *, tree_node *, tree_node *, int);
void CreateNodeContainers(screen_info *, tree_node *);
void ResizeNodeContainer(screen_info *, tree_node *);
void ApplyNodeContainer(tree_node *);
int GetOptimalSplitMode(tree_node *);

tree_node *CreateTreeFromWindowIDList(screen_info *, std::vector<int>);
tree_node *CreateRootNode();
tree_node *CreateLeafNode(screen_info *, tree_node *, int, int);
void CreateLeafNodePair(screen_info *, tree_node *, int, int, int);
void DestroyNodeTree(tree_node *);

tree_node *GetNearestNodeToTheLeft(tree_node *);
tree_node *GetNearestNodeToTheRight(tree_node *);
tree_node *GetNodeFromWindowID(tree_node *, int);
void SwapNodeWindowIDs(tree_node *, tree_node *);
void SwapNodeChildPositions(tree_node *);
void AddWindowToTree(screen_info *, int WindowID);
void AddWindowToTree();
void RemoveWindowFromTree(screen_info *, int, bool);
void RemoveWindowFromTree();
bool IsLeafNode(tree_node *);

void GetActiveDisplays();
screen_info *GetDisplayOfMousePointer();
screen_info *GetDisplayOfWindow(window_info *);
std::vector<window_info*> GetAllWindowsOnDisplay(int);
std::vector<int> GetAllWindowIDsOnDisplay(int);
void CycleFocusedWindowDisplay(int);
void ChangeGapOfDisplay(const std::string &, int);
void ChangePaddingOfDisplay(const std::string &, int);

void CreateWindowNodeTree(screen_info *);
void ShouldWindowNodeTreeUpdate(screen_info *);
void ResizeWindowToContainerSize(tree_node *);
void ResizeWindowToContainerSize();
void ReflectWindowNodeTreeVertically();
void MoveContainerSplitter(int, int);

bool IsWindowBelowCursor(window_info *);
bool IsWindowFloating(int);
bool IsWindowOnActiveSpace(window_info *);
bool IsSpaceTransitionInProgress();
bool IsSpaceSystemOrFullscreen();
bool WindowsAreEqual(window_info *, window_info *);

void UpdateWindowTree();
void UpdateActiveWindowList(screen_info *);
bool FilterWindowList();
void MarkWindowContainer();
void ShiftWindowFocus(int);
void SwapFocusedWindowWithNearest(int);
void FocusWindowBelowCursor();
void ToggleFocusedWindowFloating();
void ToggleFocusedWindowFullscreen();
void ToggleFocusedWindowParentContainer();
void SetWindowDimensions(AXUIElementRef, window_info *, int, int, int, int);

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
