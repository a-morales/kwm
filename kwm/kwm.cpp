#include "kwm.h"
#include "helpers.h"
#include "daemon.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "keys.h"
#include "interpreter.h"
#include "scratchpad.h"
#include "border.h"
#include "config.h"
#include "command.h"

#include "axlib/axlib.h"

#define internal static


const std::string KwmCurrentVersion = "Kwm Version 2.2.0";
ax_state AXState = {};
ax_application *FocusedApplication;

kwm_mach KWMMach = {};
kwm_path KWMPath = {};
kwm_screen KWMScreen = {};
kwm_toggles KWMToggles = {};
kwm_focus KWMFocus = {};
kwm_mode KWMMode = {};
kwm_tiling KWMTiling = {};
kwm_cache KWMCache = {};
kwm_thread KWMThread = {};
kwm_hotkeys KWMHotkeys = {};
kwm_border FocusedBorder = {};
kwm_border MarkedBorder = {};
scratchpad Scratchpad = {};

/* TODO(koekeishiya): Need to keep track of windows on the currently active space using 'CGCopyWindowList..' to get
                      z-ordering used for focus-follows-mouse. There are also cases in which applications doesn't seem
                      to trigger a kAXWindowCreated notification, so we probably have to pick these up from this list. */
EVENT_CALLBACK(Callback_AXEvent_WindowList)
{
    if(KWMTiling.MonitorWindows)
    {
        CheckPrefixTimeout();
        if(!IsSpaceTransitionInProgress() &&
           IsActiveSpaceManaged())
        {
            // DEBUG("AXEvent_WindowList: Refresh window list");
            if(KWMScreen.Transitioning)
                KWMScreen.Transitioning = false;
            else
                UpdateWindowTree();
        }
    }

}

/* TODO(koekeishiya): Should probably be moved to a 'cursor.cpp' or similar in the future,
                      along with other cursor-related functionality we might want */
EVENT_CALLBACK(Callback_AXEvent_MouseMoved)
{
    if(!IsSpaceTransitionInProgress())
    {
        // DEBUG("AXEvent_MouseMoved: Mouse moved");
        UpdateActiveScreen();
        UpdateActiveWindowList(KWMScreen.Current);

        if(KWMMode.Focus != FocusModeDisabled &&
           KWMMode.Focus != FocusModeStandby &&
           !IsActiveSpaceFloating())
            FocusWindowBelowCursor();
    }
}

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon)
{
    switch(Type)
    {
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
        {
            if(!KWMMach.DisableEventTapInternal)
            {
                DEBUG("Restarting Event Tap");
                CGEventTapEnable(KWMMach.EventTap, true);
            }
        } break;
        case kCGEventKeyDown:
        {
            /* TODO(koekeishiya): Is there a better way to decide whether
                                  we should eat the CGEventRef or not (?) */
            if(KWMToggles.UseBuiltinHotkeys)
            {
                hotkey Eventkey = {}, *Hotkey = NULL;
                Hotkey = (hotkey *) calloc(1, sizeof(hotkey));
                if(Hotkey)
                {
                    CreateHotkeyFromCGEvent(Event, &Eventkey);
                    if(HotkeyExists(Eventkey.Mod, Eventkey.Key, Hotkey, KWMHotkeys.ActiveMode->Name))
                    {
                        AXLibConstructEvent(AXEvent_HotkeyPressed, Hotkey);
                        if(!Hotkey->Passthrough)
                            return NULL;
                    }
                }
            }
        } break;
        case kCGEventMouseMoved:
        {
            AXLibConstructEvent(AXEvent_MouseMoved, NULL);
        } break;
        default: {} break;
    }

    return Event;
}

internal bool
CheckPrivileges()
{
    bool Result = false;
    const void * Keys[] = { kAXTrustedCheckOptionPrompt };
    const void * Values[] = { kCFBooleanTrue };

    CFDictionaryRef Options;
    Options = CFDictionaryCreate(kCFAllocatorDefault,
                                 Keys, Values, sizeof(Keys) / sizeof(*Keys),
                                 &kCFCopyStringDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

    Result = AXIsProcessTrustedWithOptions(Options);
    CFRelease(Options);

    return Result;
}

internal bool
CheckArguments(int argc, char **argv)
{
    bool Result = false;

    if(argc == 2)
    {
        std::string Arg = argv[1];
        if(Arg == "--version")
        {
            std::cout << KwmCurrentVersion << std::endl;
            Result = true;
        }
    }

    return Result;
}

internal bool
GetKwmFilePath()
{
    bool Result = false;
    char PathBuf[PROC_PIDPATHINFO_MAXSIZE];
    pid_t Pid = getpid();
    int Ret = proc_pidpath(Pid, PathBuf, sizeof(PathBuf));
    if (Ret > 0)
    {
        KWMPath.FilePath = PathBuf;

        std::size_t Split = KWMPath.FilePath.find_last_of("/\\");
        KWMPath.FilePath = KWMPath.FilePath.substr(0, Split);
        Result = true;
    }

    return Result;
}

internal void
KwmClearSettings()
{
    std::map<int, CFTypeRef>::iterator It;
    for(It = KWMTiling.AllowedWindowRoles.begin(); It != KWMTiling.AllowedWindowRoles.end(); ++It)
        CFRelease(It->second);

    KWMTiling.AllowedWindowRoles.clear();
    KWMHotkeys.Modes.clear();
    KWMTiling.WindowRules.clear();
    KWMTiling.SpaceSettings.clear();
    KWMTiling.DisplaySettings.clear();
    KWMTiling.EnforcedWindows.clear();
    KWMHotkeys.ActiveMode = GetBindingMode("default");
}

internal void
KwmExecuteConfig()
{
    char *HomeP = std::getenv("HOME");
    if(!HomeP)
    {
        DEBUG("Failed to get environment variable 'HOME'");
        return;
    }

    KWMPath.EnvHome = HomeP;
    KwmParseConfig(KWMPath.ConfigFile);
}

internal void
KwmExecuteInitScript()
{
    std::string InitFile = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/init";

    struct stat Buffer;
    if(stat(InitFile.c_str(), &Buffer) == 0)
        KwmExecuteThreadedSystemCommand(InitFile);
}

internal void
SignalHandler(int Signum)
{
    ShowAllScratchpadWindows();
    DEBUG("SignalHandler() " << Signum);

    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);
    exit(Signum);
}

internal void
Fatal(const std::string &Err)
{
    std::cout << Err << std::endl;
    exit(1);
}

internal void
KwmInit()
{
    if(!CheckPrivileges())
        Fatal("Could not access OSX Accessibility!");

    if(KwmStartDaemon())
        pthread_create(&KWMThread.Daemon, NULL, &KwmDaemonHandleConnectionBG, NULL);
    else
        Fatal("Kwm: Could not start daemon..");

    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGTRAP, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGKILL, SignalHandler);
    signal(SIGINT, SignalHandler);

    KWMScreen.SplitRatio = 0.5;
    KWMScreen.SplitMode = SPLIT_OPTIMAL;
    KWMScreen.PrevSpace = -1;
    KWMScreen.DefaultOffset = CreateDefaultScreenOffset();
    KWMScreen.MaxCount = 5;
    KWMScreen.ActiveCount = 0;

    KWMToggles.EnableTilingMode = true;
    KWMToggles.UseBuiltinHotkeys = true;
    KWMToggles.UseMouseFollowsFocus = true;
    KWMTiling.OptimalRatio = 1.618;
    KWMTiling.LockToContainer = true;
    KWMTiling.MonitorWindows = true;

    KWMMode.Space = SpaceModeBSP;
    KWMMode.Focus = FocusModeAutoraise;
    KWMMode.Cycle = CycleModeScreen;

    FocusedBorder.Radius = -1;
    MarkedBorder.Radius = -1;

    KWMPath.ConfigFile = "kwmrc";
    KWMPath.ConfigFolder = ".kwm";
    KWMPath.BSPLayouts = "layouts";
    KWMHotkeys.ActiveMode = GetBindingMode("default");

    GetKwmFilePath();
    KwmExecuteConfig();
    GetActiveDisplays();
    KwmExecuteInitScript();
}

void KwmQuit()
{
    ShowAllScratchpadWindows();
    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);

    exit(0);
}

void KwmReloadConfig()
{
    KwmClearSettings();
    KwmExecuteConfig();
}

int main(int argc, char **argv)
{
    if(CheckArguments(argc, argv))
        return 0;

    KwmInit();
    KWMMach.EventMask = ((1 << kCGEventKeyDown) |
                         (1 << kCGEventMouseMoved));

    KWMMach.EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, KWMMach.EventMask, CGEventCallback, NULL);
    if(!KWMMach.EventTap || !CGEventTapIsEnabled(KWMMach.EventTap))
        Fatal("ERROR: Could not create event-tap!");

    CFRunLoopAddSource(CFRunLoopGetMain(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, KWMMach.EventTap, 0),
                       kCFRunLoopCommonModes);
    CGEventTapEnable(KWMMach.EventTap, true);

    // NOTE(koekeishiya): Initialize AXLIB
    AXLibInit(&AXState);
    AXLibStartEventLoop();
    AXLibRunningApplications();
    FocusedApplication = AXLibGetFocusedApplication();
    UpdateWindowTree();
    UpdateBorder("focused");

    /* NOTE(koekeishiya): Find connected displays and their associated spaces
    AXLibActiveDisplays();
    */

    NSApplicationLoad();
    CFRunLoopRun();
    return 0;
}
