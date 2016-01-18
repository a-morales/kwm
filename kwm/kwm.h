#ifndef KWM_H
#define KWM_H

#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libproc.h>
#include <signal.h>

#include <pthread.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct hotkey;
struct modifiers;
struct container_offset;

struct window_info;
struct window_role;
struct screen_info;
struct space_info;
struct node_container;
struct tree_node;

struct kwm_border;
struct kwm_hotkeys;
struct kwm_prefix;
struct kwm_toggles;
struct kwm_path;
struct kwm_focus;
struct kwm_screen;
struct kwm_tiling;
struct kwm_cache;
struct kwm_mode;
struct kwm_thread;

#ifdef DEBUG_BUILD
    #define DEBUG(x) std::cout << x << std::endl;
#else
    #define DEBUG(x) do {} while (0);
#endif

typedef std::chrono::time_point<std::chrono::steady_clock> kwm_time_point;

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

enum cycle_focus_option
{
    CycleModeScreen,
    CycleModeAll,
    CycleModeDisabled
};

enum space_tiling_option
{
    SpaceModeBSP,
    SpaceModeMonocle,
    SpaceModeFloating,
    SpaceModeDefault
};

enum hotkey_state
{
    HotkeyStateNone,
    HotkeyStateInclude,
    HotkeyStateExclude
};

struct modifiers
{
    bool CmdKey;
    bool AltKey;
    bool CtrlKey;
    bool ShiftKey;
};

struct hotkey
{
    std::vector<std::string> List;
    bool IsSystemCommand;
    hotkey_state State;

    modifiers Mod;
    CGKeyCode Key;
    bool Prefixed;

    std::string Command;
};

struct container_offset
{
    double PaddingTop, PaddingBottom;
    double PaddingLeft, PaddingRight;
    double VerticalGap, HorizontalGap;
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
    container_offset Offset;
    bool Initialized;

    space_tiling_option Mode;
    tree_node *RootNode;
};

struct screen_info
{
    unsigned int ID;
    int X, Y;
    double Width, Height;
    container_offset Offset;

    int ActiveSpace;
    int OldWindowListCount;
    bool ForceContainerUpdate;
    std::map<int, space_info> Space;
};

struct kwm_border
{
    FILE *FHandle;
    FILE *MHandle;
    bool FEnabled;
    bool MEnabled;

    int FWidth;
    unsigned int FColor;
    kwm_time_point FTime;

    int MWidth;
    unsigned int MColor;
    kwm_time_point MTime;
};

struct kwm_prefix
{
    kwm_time_point Time;
    hotkey Key;

    double Timeout;
    bool Enabled;
    bool Active;
    bool Global;
};

struct kwm_hotkeys
{
    std::vector<hotkey> List;
    kwm_prefix Prefix;
};

struct kwm_toggles
{
    bool UseMouseFollowsFocus;
    bool WindowDragInProgress;
    bool EnableTilingMode;
    bool UseBuiltinHotkeys;
    bool EnableDragAndDrop;
    bool UseContextMenuFix;
};

struct kwm_path
{
    std::string EnvHome;
    std::string FilePath;
    std::string ConfigFolder;
    std::string ConfigFile;
    std::string HotkeySOFullPath;
};

struct kwm_focus
{
    AXObserverRef Observer;
    AXUIElementRef Application;

    ProcessSerialNumber PSN;
    window_info *Window;
    window_info Cache;
};

struct kwm_screen
{
    screen_info *Current;
    bool ForceRefreshFocus;
    double SplitRatio;

    unsigned int OldScreenID;
    bool UpdateSpace;
    int MarkedWindow;
    int SplitMode;
    int PrevSpace;

    container_offset DefaultOffset;
    CGDirectDisplayID *Displays;
    unsigned int MaxCount;
    unsigned int ActiveCount;
};

struct kwm_tiling
{
    bool NonZeroLayer;
    bool SpawnAsLeftChild;
    bool FloatNonResizable;
    std::map<unsigned int, screen_info> DisplayMap;
    std::map<unsigned int, space_tiling_option> DisplayMode;

    std::map<std::string, std::vector<CFTypeRef> > AllowedWindowRoles;
    std::map<std::string, int> CapturedAppLst;
    std::vector<std::string> FloatingAppLst;

    std::vector<window_info> WindowLst;
    std::vector<int> FloatingWindowLst;
};

struct kwm_cache
{
    std::map<int, window_role> WindowRole;
    std::map<int, std::vector<AXUIElementRef> > WindowRefs;
};

struct kwm_mode
{
    space_tiling_option Space;
    cycle_focus_option Cycle;
    focus_option Focus;
};

struct kwm_thread
{
    pthread_t WindowMonitor;
    pthread_t Daemon;
    pthread_mutex_t Lock;
};

container_offset CreateDefaultScreenOffset();
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
tree_node *GetNearestLeafNeighbour(tree_node *, space_tiling_option);
tree_node *GetFirstPseudoLeafNode(tree_node *);
void SwapNodeWindowIDs(tree_node *, tree_node *);
void AddWindowToTreeOfUnfocusedMonitor(screen_info *, window_info *);
bool IsLeafNode(tree_node *);
bool IsLeftChild(tree_node *);
bool IsRightChild(tree_node *);

screen_info CreateDefaultScreenInfo(int, int);
void UpdateExistingScreenInfo(screen_info *, int, int);
void DisplayReconfigurationCallBack(CGDirectDisplayID, CGDisplayChangeSummaryFlags, void *);
void GetActiveDisplays();
void RefreshActiveDisplays();
void SetSpaceModeOfDisplay(unsigned int, std::string);
space_tiling_option GetSpaceModeOfDisplay(unsigned int);
screen_info *GetDisplayOfMousePointer();
screen_info *GetDisplayOfWindow(window_info *);
std::vector<window_info*> GetAllWindowsOnDisplay(int);
std::vector<int> GetAllWindowIDsOnDisplay(int);
bool DoesSpaceExistInMapOfScreen(screen_info *);
bool IsSpaceInitializedForScreen(screen_info *);
screen_info *GetDisplayFromScreenID(unsigned int);
int GetIndexOfPrevScreen();
int GetIndexOfNextScreen();
void GiveFocusToScreen(int, tree_node *, bool);
void ActivateScreen(screen_info *, bool);
void MoveWindowToDisplay(window_info *, int, bool);
void CaptureApplicationToScreen(int, std::string);

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
bool IsAnyWindowBelowCursor();
bool IsSpaceFloating(int);
bool IsApplicationFloating(window_info *);
bool IsApplicationCapturedByScreen(window_info *);
void CaptureApplication(window_info *);
bool IsWindowFloating(int, int *);
bool IsWindowOnActiveSpace(int);
bool IsSpaceTransitionInProgress();
bool IsSpaceSystemOrFullscreen();
bool IsContextMenusAndSimilarVisible();
bool WindowsAreEqual(window_info *, window_info *);
bool IsAppSpecificWindowRole(window_info *, CFTypeRef, CFTypeRef);
void AllowRoleForApplication(std::string, std::string);

void UpdateWindowTree();
void UpdateActiveWindowList(screen_info *);
bool FilterWindowList(screen_info *);
void MarkWindowContainer();
void ClearMarkedWindow();
void ShiftWindowFocus(int);
void SwapFocusedWindowWithNearest(int);
void SwapFocusedWindowWithMarked();
void FocusWindowBelowCursor();
bool FocusWindowOfOSX();
void FloatFocusedSpace();
void TileFocusedSpace(space_tiling_option);
void ToggleFocusedSpaceFloating();
void ToggleWindowFloating(int);
void ToggleFocusedWindowFloating();
void ToggleFocusedWindowFullscreen();
void ToggleFocusedWindowParentContainer();
void SetWindowDimensions(AXUIElementRef, window_info *, int, int, int, int);
void CenterWindow(screen_info *, window_info *);
void ModifyContainerSplitRatio(double);
void ResizeWindowToContainerSize(tree_node *);
void ResizeWindowToContainerSize();

bool IsCursorInsideFocusedWindow();
CGPoint GetCursorPos();

kwm_time_point PerformUpdateBorderTimer(kwm_time_point);
void UpdateBorder(std::string);
void ClearFocusedBorder();
void CloseWindowByRef(AXUIElementRef);
void CloseWindow(window_info *);
void SetWindowRefFocus(AXUIElementRef, window_info *);
void SetWindowFocus(window_info *);
void SetWindowFocusByNode(tree_node *);
void MoveCursorToCenterOfFocusedWindow();
void MoveCursorToCenterOfWindow(window_info *);

std::string GetWindowTitle(AXUIElementRef);
CGPoint GetWindowPos(AXUIElementRef);
CGSize GetWindowSize(AXUIElementRef);
window_info *GetWindowByID(int);
bool GetWindowRef(window_info *, AXUIElementRef *);
bool GetWindowRole(window_info *, CFTypeRef *, CFTypeRef *);
void GetWindowInfo(const void *, const void *, void *);
bool GetWindowFocusedByOSX(int *);
bool IsApplicationInCache(int, std::vector<AXUIElementRef> *);
bool GetWindowRefFromCache(window_info *, AXUIElementRef *);
void FreeWindowRefCache(int);

bool KwmStartDaemon();
void KwmDaemonHandleConnection();
void * KwmDaemonHandleConnectionBG(void *);
void KwmTerminateDaemon();

void SaveBSPTreeToFile(screen_info *, std::string);
void LoadBSPTreeFromFile(screen_info *, std::string);
void SerializeParentNode(tree_node *, std::string, std::vector<std::string> &);
tree_node *DeserializeNodeTree(std::vector<std::string> &);
unsigned int DeserializeChildNode(tree_node *, std::vector<std::string> &, unsigned int);
unsigned int DeserializeParentNode(tree_node *, std::vector<std::string> &, unsigned int);
void FillDeserializedTree(tree_node *);
void CreateDeserializedNodeContainer(tree_node *);
int ConvertStringToInt(std::string);
double ConvertStringToDouble(std::string);

std::string KwmReadFromSocket(int);
void KwmWriteToSocket(int, std::string);
void KwmInterpretCommand(std::string, int);
std::vector<std::string> SplitString(std::string, char);
bool IsPrefixOfString(std::string &, std::string);
std::string CreateStringFromTokens(std::vector<std::string>, int);

CFStringRef KeycodeToString(CGKeyCode);
bool KeycodeForChar(char, CGKeyCode *);
bool GetLayoutIndependentKeycode(std::string, CGKeyCode *);
bool KwmMainHotkeyTrigger(CGEventRef *);
bool KwmIsPrefixKey(hotkey *, modifiers *, CGKeyCode);
bool KwmParseHotkey(std::string, std::string, hotkey *);
bool HotkeysAreEqual(hotkey *, hotkey *);
void DetermineHotkeyState(hotkey *, std::string &);
bool KwmExecuteHotkey(modifiers, CGKeyCode);
bool HotkeyExists(modifiers, CGKeyCode, hotkey *);
void KwmAddHotkey(std::string, std::string);
void KwmRemoveHotkey(std::string);

void CreateApplicationNotifications();
void DestroyApplicationNotifications();
void FocusedAXObserverCallback(AXObserverRef, AXUIElementRef, CFStringRef, void *);

void KwmInit();
void KwmQuit();
void SignalHandler(int);
void KwmSetPrefix(std::string);
void KwmSetPrefixGlobal(bool);
void KwmSetPrefixTimeout(double);
bool GetTagForCurrentSpace(std::string &);
bool GetKwmFilePath();
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
