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

const std::string KwmCurrentVersion = "Kwm Version 2.2.0";

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
kwm_callback KWMCallback =  {};
scratchpad Scratchpad = {};

std::map<pid_t, ax_application> AXApplications;

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
            if(KWMToggles.UseBuiltinHotkeys)
            {
                hotkey Eventkey = {}, Hotkey = {};
                CreateHotkeyFromCGEvent(Event, &Eventkey);
                if(HotkeyExists(Eventkey.Mod, Eventkey.Key, &Hotkey, KWMHotkeys.ActiveMode->Name))
                {
                    KWMHotkeys.Queue.push(Hotkey);
                    if(!Hotkey.Passthrough)
                        return NULL;
                }
            }

            if(KWMMode.Focus == FocusModeAutofocus &&
               !IsActiveSpaceFloating())
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&KWMFocus.PSN, Event);
                return NULL;
            }
        } break;
        case kCGEventKeyUp:
        {
            if(KWMMode.Focus == FocusModeAutofocus &&
               !IsActiveSpaceFloating())
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&KWMFocus.PSN, Event);
                return NULL;
            }
        } break;
        case kCGEventMouseMoved:
        {
            pthread_mutex_lock(&KWMThread.Lock);
            if(!IsSpaceTransitionInProgress())
            {
                UpdateActiveScreen();

                if(KWMMode.Focus != FocusModeDisabled &&
                   KWMMode.Focus != FocusModeStandby &&
                   !IsActiveSpaceFloating())
                    FocusWindowBelowCursor();
            }
            pthread_mutex_unlock(&KWMThread.Lock);
        } break;
        default: {} break;
    }

    return Event;
}

void KwmQuit()
{
    ShowAllScratchpadWindows();
    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);

    exit(0);
}

void * KwmWindowMonitor(void*)
{
    while(1)
    {
        if(KWMTiling.MonitorWindows)
        {
            pthread_mutex_lock(&KWMThread.Lock);
            CheckPrefixTimeout();
            if(!IsSpaceTransitionInProgress() &&
               IsActiveSpaceManaged())
            {
                if(KWMScreen.Transitioning)
                    KWMScreen.Transitioning = false;
                else
                    UpdateWindowTree();
            }

            pthread_mutex_unlock(&KWMThread.Lock);
        }

        usleep(200000);
    }
}

void KwmReloadConfig()
{
    KwmClearSettings();
    KwmExecuteConfig();
}

void KwmClearSettings()
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

void KwmExecuteConfig()
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

void KwmExecuteInitScript()
{
    std::string InitFile = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/init";

    struct stat Buffer;
    if(stat(InitFile.c_str(), &Buffer) == 0)
        KwmExecuteThreadedSystemCommand(InitFile);
}

bool GetKwmFilePath()
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

void KwmInit()
{
    if(!CheckPrivileges())
        Fatal("Could not access OSX Accessibility!");

    if (pthread_mutex_init(&KWMThread.Lock, NULL) != 0)
        Fatal("Could not create mutex!");

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

    pthread_create(&KWMThread.WindowMonitor, NULL, &KwmWindowMonitor, NULL);
    pthread_create(&KWMThread.Hotkey, NULL, &KwmMainHotkeyTrigger, NULL);
    FocusWindowOfOSX();
}

bool CheckPrivileges()
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

bool CheckArguments(int argc, char **argv)
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

void SignalHandler(int Signum)
{
    ShowAllScratchpadWindows();
    DEBUG("SignalHandler() " << Signum);

    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);
    exit(Signum);
}

void Fatal(const std::string &Err)
{
    std::cout << Err << std::endl;
    exit(1);
}

int main(int argc, char **argv)
{
    if(CheckArguments(argc, argv))
        return 0;

    KwmInit();
    KWMMach.EventMask = ((1 << kCGEventKeyDown) |
                         (1 << kCGEventKeyUp) |
                         (1 << kCGEventMouseMoved));

    KWMMach.EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, KWMMach.EventMask, CGEventCallback, NULL);
    if(!KWMMach.EventTap || !CGEventTapIsEnabled(KWMMach.EventTap))
        Fatal("ERROR: Could not create event-tap!");

    CFRunLoopAddSource(CFRunLoopGetMain(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, KWMMach.EventTap, 0),
                       kCFRunLoopCommonModes);

    CGEventTapEnable(KWMMach.EventTap, true);
    CreateWorkspaceWatcher(KWMMach.WorkspaceWatcher);

    // NOTE(koekeishiya): Initialize AXLIB
    // AXLibRunningApplications(&AXApplications);

    NSApplicationLoad();
    CFRunLoopRun();
    return 0;
}
