#ifndef TYPES_H
#define TYPES_H

#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <queue>
#include <stack>
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

struct token;
struct tokenizer;
struct space_identifier;
struct color;
struct hotkey;
struct modifiers;
struct space_settings;
struct container_offset;

struct window_properties;
struct window_rule;
struct window_info;
struct window_role;
struct screen_info;
struct space_info;
struct node_container;
struct tree_node;

struct kwm_mach;
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
    #define DEBUG(x) std::cout << x << std::endl
    #define Assert(Expression) do \
                               { if(!(Expression)) \
                                   {\
                                       std::cout << "Assertion failed: " << #Expression << std::endl;\
                                       *(volatile int*)0 = 0;\
                                   } \
                               } while(0)
#else
    #define DEBUG(x) do {} while(0)
    #define Assert(Expression) do {} while(0)
#endif

#define BSP_WINDOW_EVENT_CALLBACK(name) void name(window_info *Window, int OpenWindows)
typedef BSP_WINDOW_EVENT_CALLBACK(OnBSPWindowCreate);
typedef BSP_WINDOW_EVENT_CALLBACK(OnBSPWindowDestroy);

typedef std::chrono::time_point<std::chrono::steady_clock> kwm_time_point;

#define CGSSpaceTypeUser 0
extern "C" int CGSGetActiveSpace(int cid);
extern "C" int CGSSpaceGetType(int cid, int sid);
extern "C" bool CGSManagedDisplayIsAnimating(const int cid, CFStringRef display);
extern "C" CFStringRef CGSCopyManagedDisplayForSpace(const int cid, int space);
extern "C" CFStringRef CGSCopyBestManagedDisplayForRect(const int cid, CGRect rect);
extern "C" CFArrayRef CGSCopyManagedDisplaySpaces(const int cid);
extern "C" CFArrayRef CGSCopySpacesForWindows(int cid, int type, CFArrayRef windows);


#define CGSDefaultConnection _CGSDefaultConnection()
extern "C" int _CGSDefaultConnection(void);

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, int *);

enum focus_option
{
    FocusModeAutofocus,
    FocusModeAutoraise,
    FocusModeStandby,
    FocusModeDisabled
};

enum cycle_focus_option
{
    CycleModeScreen,
    CycleModeDisabled
};

enum space_tiling_option
{
    SpaceModeBSP,
    SpaceModeMonocle,
    SpaceModeFloating,
    SpaceModeDefault
};

enum split_type
{
    SPLIT_OPTIMAL = -1,
    SPLIT_VERTICAL = 1,
    SPLIT_HORIZONTAL = 2
};

enum node_type
{
    NodeTypeTree,
    NodeTypeLink
};

enum hotkey_state
{
    HotkeyStateNone,
    HotkeyStateInclude,
    HotkeyStateExclude
};

enum token_type
{
    Token_Colon,
    Token_SemiColon,
    Token_Equals,
    Token_Dash,

    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,

    Token_Identifier,
    Token_String,

    Token_EndOfStream,
    Token_Unknown,
};

struct token
{
    token_type Type;

    int TextLength;
    char *Text;
};

struct tokenizer
{
    char *At;
};

struct space_identifier
{

    int ScreenID, SpaceID;

    bool operator<(const space_identifier &Other) const
    {
        return (ScreenID < Other.ScreenID) ||
               (ScreenID == Other.ScreenID && SpaceID < Other.SpaceID);
    }
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
    bool Passthrough;

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

struct color
{
    double Red;
    double Green;
    double Blue;
    double Alpha;

    std::string Format;
};

struct node_container
{
    double X, Y;
    double Width, Height;
    int Type;
};

struct link_node
{
    int WindowID;
    node_container Container;

    link_node *Prev;
    link_node *Next;
};

struct tree_node
{
    int WindowID;
    node_type Type;
    node_container Container;

    link_node *List;

    tree_node *Parent;
    tree_node *LeftChild;
    tree_node *RightChild;

    split_type SplitMode;
    double SplitRatio;
};

struct window_properties
{
    int Display;
    int Space;
    int Float;
};

struct window_rule
{
    window_properties Properties;
    std::string Except;
    std::string Owner;
    std::string Name;
};

struct window_info
{
    std::string Name;
    std::string Owner;
    int PID, WID;
    int Layer;
    int X, Y;
    int Width, Height;

    bool Float;
    unsigned int Space;
    unsigned int Display;
};

struct window_role
{
    CFTypeRef Role;
    CFTypeRef SubRole;
};

struct space_settings
{
    container_offset Offset;
    space_tiling_option Mode;
};

struct space_info
{
    space_settings Settings;
    bool Initialized;
    bool Managed;
    bool NeedsUpdate;

    tree_node *RootNode;
    int FocusedWindowID;
};

struct screen_info
{
    CFStringRef Identifier;
    unsigned int ID;

    int X, Y;
    double Width, Height;
    space_settings Settings;

    int ActiveSpace;
    bool RestoreFocus;
    bool TrackSpaceChange;
    std::stack<int> History;
    std::map<int, space_info> Space;
};

struct kwm_mach
{
    void *WorkspaceWatcher;
    CFRunLoopSourceRef RunLoopSource;
    CFMachPortRef EventTap;
    CGEventMask EventMask;
    bool DisableEventTapInternal;
};

struct kwm_border
{
    bool Enabled;
    FILE *Handle;

    double Radius;
    color Color;
    int Width;
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
    std::queue<hotkey> Queue;
    std::vector<hotkey> List;
    kwm_prefix Prefix;
    modifiers SpacesKey;
};

struct kwm_toggles
{
    bool UseMouseFollowsFocus;
    bool EnableTilingMode;
    bool UseBuiltinHotkeys;
    bool StandbyOnFloat;
};

struct kwm_path
{
    std::string EnvHome;
    std::string FilePath;
    std::string ConfigFolder;
    std::string ConfigFile;
    std::string BSPLayouts;
};

struct kwm_focus
{
    AXObserverRef Observer;
    AXUIElementRef Application;

    ProcessSerialNumber PSN;
    window_info *Window;
    window_info Cache;
    window_info NULLWindowInfo;
    window_info InsertionPoint;
};

struct kwm_screen
{
    screen_info *Current;
    bool Transitioning;
    double SplitRatio;

    int MarkedWindow;
    split_type SplitMode;
    int PrevSpace;

    container_offset DefaultOffset;
    CGDirectDisplayID *Displays;
    unsigned int MaxCount;
    unsigned int ActiveCount;
};

struct kwm_tiling
{
    bool MonitorWindows;
    double OptimalRatio;
    bool SpawnAsLeftChild;
    bool FloatNonResizable;
    bool LockToContainer;

    std::map<unsigned int, screen_info> DisplayMap;
    std::map<unsigned int, space_settings> DisplaySettings;
    std::map<space_identifier, space_settings> SpaceSettings;

    std::vector<window_info> FocusLst;
    std::vector<window_info> WindowLst;
    std::vector<int> FloatingWindowLst;


    std::map<std::string, std::vector<CFTypeRef> > AllowedWindowRoles;
    std::vector<window_rule> WindowRules;
    std::map<int, bool> EnforcedWindows;
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
    pthread_t SystemCommand;
    pthread_t Hotkey;
    pthread_t Daemon;
    pthread_mutex_t Lock;
};

struct kwm_callback
{
    OnBSPWindowCreate *WindowCreate;
    OnBSPWindowDestroy *WindowDestroy;
};

#endif
