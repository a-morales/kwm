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
#include "cursor.h"
#include "axlib/axlib.h"

#define internal static
const std::string KwmCurrentVersion = "Kwm Version 3.0.1";
std::map<const char *, space_info> WindowTree;

ax_state AXState = {};
ax_display *FocusedDisplay = NULL;
ax_application *FocusedApplication = NULL;
ax_window *MarkedWindow = NULL;

kwm_mach KWMMach = {};
kwm_path KWMPath = {};
kwm_settings KWMSettings = {};
kwm_thread KWMThread = {};
kwm_hotkeys KWMHotkeys = {};
kwm_border FocusedBorder = {};
kwm_border MarkedBorder = {};
scratchpad Scratchpad = {};

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
            if(KWMSettings.UseBuiltinHotkeys)
            {
                hotkey Eventkey = {}, *Hotkey = NULL;
                Hotkey = new hotkey;
                if(Hotkey)
                {
                    CreateHotkeyFromCGEvent(Event, &Eventkey);
                    if(HotkeyExists(Eventkey.Mod, Eventkey.Key, Hotkey, KWMHotkeys.ActiveMode->Name))
                    {
                        AXLibConstructEvent(AXEvent_HotkeyPressed, Hotkey, false);
                        if(!Hotkey->Passthrough)
                            return NULL;
                    }
                    else
                    {
                        delete Hotkey;
                    }
                }
            }
        } break;
        case kCGEventMouseMoved:
        {
            if(KWMSettings.Focus == FocusModeAutoraise)
                AXLibConstructEvent(AXEvent_MouseMoved, NULL, false);
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
    KWMHotkeys.Modes.clear();
    KWMSettings.WindowRules.clear();
    KWMSettings.SpaceSettings.clear();
    KWMSettings.DisplaySettings.clear();
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
        Fatal("Error: Could not access OSX Accessibility!");

    if(KwmStartDaemon())
        pthread_create(&KWMThread.Daemon, NULL, &KwmDaemonHandleConnectionBG, NULL);
    else
        Fatal("Error: Could not start daemon!");

    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGTRAP, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGKILL, SignalHandler);
    signal(SIGINT, SignalHandler);

    KWMSettings.SplitRatio = 0.5;
    KWMSettings.SplitMode = SPLIT_OPTIMAL;
    KWMSettings.DefaultOffset = CreateDefaultScreenOffset();

    KWMSettings.UseBuiltinHotkeys = true;
    KWMSettings.UseMouseFollowsFocus = true;
    KWMSettings.OptimalRatio = 1.618;
    KWMSettings.LockToContainer = true;

    KWMSettings.Space = SpaceModeBSP;
    KWMSettings.Focus = FocusModeAutoraise;
    KWMSettings.Cycle = CycleModeScreen;

    FocusedBorder.Radius = -1;
    MarkedBorder.Radius = -1;

    KWMPath.ConfigFile = "kwmrc";
    KWMPath.ConfigFolder = ".kwm";
    KWMPath.BSPLayouts = "layouts";
    KWMHotkeys.ActiveMode = GetBindingMode("default");

    GetKwmFilePath();
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

    NSApplicationLoad();
    if(!AXLibDisplayHasSeparateSpaces())
        Fatal("Error: 'Displays have separate spaces' must be enabled!");

    /* NOTE(koekeishiya): Initialize AXLIB */
    AXLibInit(&AXState);
    AXLibStartEventLoop();

    ax_display *MainDisplay = AXLibMainDisplay();
    ax_display *Display = MainDisplay;
    do
    {
        ax_space *PrevSpace = Display->Space;
        Display->Space = AXLibGetActiveSpace(Display);
        Display->PrevSpace = PrevSpace;
        Display = AXLibNextDisplay(Display);
    } while(Display != MainDisplay);

    FocusedDisplay = MainDisplay;
    FocusedApplication = AXLibGetFocusedApplication();
    /* ----------------------------------- */

    KwmInit();
    KwmExecuteConfig();
    KwmExecuteInitScript();
    CreateWindowNodeTree(MainDisplay);

    if(CGSIsSecureEventInputSet())
        fprintf(stderr, "Secure Keyboard Entry is enabled, hotkeys will not work!\n");

    KWMMach.EventMask = ((1 << kCGEventKeyDown) |
                         (1 << kCGEventMouseMoved));

    KWMMach.EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, KWMMach.EventMask, CGEventCallback, NULL);
    if(!KWMMach.EventTap || !CGEventTapIsEnabled(KWMMach.EventTap))
        Fatal("Error: Could not create event-tap!");

    CFRunLoopAddSource(CFRunLoopGetMain(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, KWMMach.EventTap, 0),
                       kCFRunLoopCommonModes);
    CGEventTapEnable(KWMMach.EventTap, true);
    CFRunLoopRun();
    return 0;
}
