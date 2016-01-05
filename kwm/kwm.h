#ifndef KWM_H
#define KWM_H

#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <string>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libproc.h>

#include <pthread.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct kwm_code;
struct hotkey;
struct modifiers;

struct window_info;
struct window_role;
struct screen_info;
struct space_info;
struct node_container;
struct tree_node;

#ifdef DEBUG_BUILD
    #define DEBUG(x) std::cout << x << std::endl;
#else
    #define DEBUG(x) do {} while (0);
#endif

#define KWM_HOTKEY_COMMANDS(name) bool name(modifiers Mod, CGKeyCode Keycode)
typedef KWM_HOTKEY_COMMANDS(kwm_hotkey_commands);

#define KWM_KEY_REMAP(name) void name(modifiers *Mod, CGKeyCode Keycode, int *Result)
typedef KWM_KEY_REMAP(kwm_key_remap);

#define CGSSpaceTypeUser 0
extern "C" int CGSGetActiveSpace(int cid);
extern "C" int CGSSpaceGetType(int cid, int sid);
extern "C" bool CGSManagedDisplayIsAnimating(const int cid, CFStringRef display);
extern "C" CFStringRef CGSCopyManagedDisplayForSpace(const int cid, int space);

#define CGSDefaultConnection _CGSDefaultConnection()
extern "C" int _CGSDefaultConnection(void);

extern "C" void NSApplicationLoad(void);
extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, int *);

enum focus_option
{ 
    FocusModeAutofocus, 
    FocusModeAutoraise, 
    FocusModeDisabled 
};

enum space_tiling_option
{
    SpaceModeBSP,
    SpaceModeMonocle,
    SpaceModeFloating
};

struct modifiers
{
    bool CmdKey;
    bool AltKey;
    bool CtrlKey;
    bool ShiftKey;
};

struct kwm_code
{
    void *KwmHotkeySO;
    std::string HotkeySOFileTime;

    kwm_hotkey_commands *KWMHotkeyCommands;
    kwm_key_remap *RemapKeys;

    bool IsValid;
};

struct hotkey
{
    bool IsSystemCommand;

    modifiers Mod;
    CGKeyCode Key;

    std::string Command;
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

    int SplitMode;
    double SplitRatio;
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

struct window_role
{
    CFTypeRef Role;
    CFTypeRef SubRole;
};

struct space_info
{
    double PaddingTop, PaddingBottom;
    double PaddingLeft, PaddingRight;
    double VerticalGap, HorizontalGap;
    bool Initialized;

    space_tiling_option Mode;
    tree_node *RootNode;
};

struct screen_info
{
    int ID;
    int X, Y;
    double Width, Height;
    double PaddingTop, PaddingBottom;
    double PaddingLeft, PaddingRight;
    double VerticalGap, HorizontalGap;

    int ActiveSpace;
    int OldWindowListCount;
    bool ForceContainerUpdate;
    std::map<int, space_info> Space;
};

node_container LeftVerticalContainerSplit(screen_info *, tree_node *);
node_container RightVerticalContainerSplit(screen_info *, tree_node *);
node_container UpperHorizontalContainerSplit(screen_info *, tree_node *);
node_container LowerHorizontalContainerSplit(screen_info *, tree_node *);

void SetRootNodeContainer(screen_info *, tree_node *);
void CreateNodeContainer(screen_info *, tree_node *, int);
void CreateNodeContainerPair(screen_info *, tree_node *, tree_node *, int);
void CreateNodeContainers(screen_info *, tree_node *, bool);
void ResizeNodeContainer(screen_info *, tree_node *);
void ApplyNodeContainer(tree_node *, space_tiling_option);
int GetOptimalSplitMode(tree_node *);
void ChangeSplitRatio(double);
void ToggleNodeSplitMode(screen_info *, tree_node *);

tree_node *CreateTreeFromWindowIDList(screen_info *, std::vector<window_info*> *);
bool CreateBSPTree(tree_node *, screen_info *, std::vector<window_info*> *);
bool CreateMonocleTree(tree_node *, screen_info *, std::vector<window_info*> *);
void RotateTree(tree_node *, int);
void DestroyNodeTree(tree_node *, space_tiling_option);
tree_node *CreateRootNode();
tree_node *CreateLeafNode(screen_info *, tree_node *, int, int);
void CreateLeafNodePair(screen_info *, tree_node *, int, int, int);

tree_node *GetNearestNodeToTheLeft(tree_node *, space_tiling_option);
tree_node *GetNearestNodeToTheRight(tree_node *, space_tiling_option);
tree_node *GetNodeFromWindowID(tree_node *, int, space_tiling_option);
tree_node *GetFirstLeafNode(tree_node *);
tree_node *GetLastLeafNode(tree_node *);
void SwapNodeWindowIDs(tree_node *, tree_node *);
void AddWindowToTreeOfUnfocusedMonitor(screen_info *);
bool IsLeafNode(tree_node *);

screen_info CreateDefaultScreenInfo(int, int);
void UpdateExistingScreenInfo(screen_info *, int, int);
void DisplayReconfigurationCallBack(CGDirectDisplayID, CGDisplayChangeSummaryFlags, void *);
void GetActiveDisplays();
void RefreshActiveDisplays();
screen_info *GetDisplayOfMousePointer();
screen_info *GetDisplayOfWindow(window_info *);
std::vector<window_info*> GetAllWindowsOnDisplay(int);
std::vector<int> GetAllWindowIDsOnDisplay(int);
bool DoesSpaceExistInMapOfScreen(screen_info *);
void CycleFocusedWindowDisplay(int, bool);

void ChangeGapOfDisplay(const std::string &, int);
void ChangePaddingOfDisplay(const std::string &, int);
void SetDefaultPaddingOfDisplay(const std::string &, int);
void SetDefaultGapOfDisplay(const std::string &, int);

void CreateWindowNodeTree(screen_info *, std::vector<window_info*> *);
void ShouldWindowNodeTreeUpdate(screen_info *);

void ShouldBSPTreeUpdate(screen_info *, space_info *);
void AddWindowToBSPTree(screen_info *, int);
void AddWindowToBSPTree();
void RemoveWindowFromBSPTree(screen_info *, int, bool);
void RemoveWindowFromBSPTree();

void ShouldMonocleTreeUpdate(screen_info *, space_info *);
void AddWindowToMonocleTree(screen_info *, int);
void RemoveWindowFromMonocleTree(screen_info *, int, bool);

bool IsWindowBelowCursor(window_info *);
bool IsSpaceFloating(int);
bool IsApplicationFloating(window_info *);
bool IsWindowFloating(int, int *);
bool IsWindowOnActiveSpace(window_info *);
bool IsSpaceTransitionInProgress();
bool IsSpaceSystemOrFullscreen();
bool IsContextMenusAndSimilarVisible();
bool WindowsAreEqual(window_info *, window_info *);
bool IsAppSpecificWindowRole(window_info *, CFTypeRef, CFTypeRef);
void AllowRoleForApplication(std::string, std::string);

void UpdateWindowTree();
void UpdateActiveWindowList(screen_info *);
bool FilterWindowList();
void MarkWindowContainer();
void ShiftWindowFocus(int);
void SwapFocusedWindowWithNearest(int);
void SwapFocusedWindowWithMarked();
void FocusWindowBelowCursor();
void FloatFocusedSpace();
void TileFocusedSpace(space_tiling_option);
void ToggleFocusedSpaceFloating();
void ToggleFocusedWindowFloating();
void ToggleFocusedWindowFullscreen();
void ToggleFocusedWindowParentContainer();
void SetWindowDimensions(AXUIElementRef, window_info *, int, int, int, int);
void CenterWindow(screen_info *);
void ModifyContainerSplitRatio(double);
void ResizeWindowToContainerSize(tree_node *);
void ResizeWindowToContainerSize();

bool IsCursorInsideFocusedWindow();
CGPoint GetCursorPos();

void CloseWindowByRef(AXUIElementRef);
void CloseWindow(window_info *);
void SetWindowRefFocus(AXUIElementRef, window_info *);
void SetWindowFocus(window_info *);
void MoveCursorToCenterOfFocusedWindow();

std::string GetWindowTitle(AXUIElementRef);
CGPoint GetWindowPos(AXUIElementRef);
CGSize GetWindowSize(AXUIElementRef);
window_info *GetWindowByID(int);
bool GetWindowRef(window_info *, AXUIElementRef *);
bool GetWindowRole(window_info *, CFTypeRef *, CFTypeRef *);
void GetWindowInfo(const void *, const void *, void *);
bool IsApplicationInCache(int, std::vector<AXUIElementRef> *);
bool GetWindowRefFromCache(window_info *, AXUIElementRef *);
void FreeWindowRefCache(int);

kwm_code LoadKwmCode();
void UnloadKwmCode(kwm_code *);
std::string KwmGetFileTime(const char *);

bool KwmStartDaemon();
void KwmDaemonHandleConnection();
void * KwmDaemonHandleConnectionBG(void *);
void KwmTerminateDaemon();

std::string KwmReadFromSocket(int);
void KwmWriteToSocket(int, std::string);
void KwmInterpretCommand(std::string, int);
std::vector<std::string> SplitString(std::string, char);
bool IsPrefixOfString(std::string &, std::string);
std::string CreateStringFromTokens(std::vector<std::string>, int);

bool KwmRunLiveCodeHotkeySystem(CGEventRef *, modifiers *, CGKeyCode);
CFStringRef KeycodeToString(CGKeyCode);
bool KeycodeForChar(char, CGKeyCode *);
bool GetLayoutIndependentKeycode(std::string, CGKeyCode *);
bool KwmParseHotkey(std::string, std::string, hotkey *);
bool KwmExecuteHotkey(modifiers, CGKeyCode);
bool HotkeyExists(modifiers, CGKeyCode, hotkey *);
void KwmAddHotkey(std::string, std::string);
void KwmRemoveHotkey(std::string);

void KwmInit();
void KwmQuit();
bool GetTagForCurrentSpace(std::string &);
void GetKwmFilePath();
bool IsKwmAlreadyAddedToLaunchd();
void AddKwmToLaunchd();
void RemoveKwmFromLaunchd();
void KwmEmitKeystrokes(std::string);
void KwmReloadConfig();
void KwmClearSettings();
void KwmExecuteConfig();
bool CheckArguments(int, char **);
bool CheckPrivileges();
void Fatal(const std::string &);
CGEventRef CGEventCallback(CGEventTapProxy, CGEventType, CGEventRef, void *);

#endif
