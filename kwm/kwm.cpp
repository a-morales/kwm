#include "kwm.h"
#include "helpers.h"
#include "daemon.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "keys.h"
#include "interpreter.h"
#include "border.h"

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

void KwmDefineVariable(std::map<std::string, std::string> &Defines, std::string Line)
{
    std::vector<std::string> Tokens = SplitString(Line, ' ');
    std::string Value = CreateStringFromTokens(Tokens, 1);
    Defines[Tokens[0]] = Value;
}

void KwmSubstitueVariables(std::map<std::string, std::string> &Defines, std::string &Line)
{
    if(Line.substr(0, 6) == "define")
        return;

    std::map<std::string, std::string>::iterator It;
    for(It = Defines.begin(); It != Defines.end(); ++It)
    {
        std::size_t Pos = Line.find(It->first);
        if(Pos != std::string::npos)
        {
            DEBUG("Replacing " << It->first << " with " << It->second);
            Line.replace(Pos, It->first.size(), It->second);
        }
    }
}

void KwmReloadConfig()
{
    KwmClearSettings();
    KwmExecuteConfig();
}

void KwmClearSettings()
{
    std::map<std::string, std::vector<CFTypeRef> >::iterator It;
    for(It = KWMTiling.AllowedWindowRoles.begin(); It != KWMTiling.AllowedWindowRoles.end(); ++It)
    {
        std::vector<CFTypeRef> &WindowRoles = It->second;
        for(std::size_t RoleIndex = 0; RoleIndex < WindowRoles.size(); ++RoleIndex)
            CFRelease(WindowRoles[RoleIndex]);

        WindowRoles.clear();
    }

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
    KwmExecuteFile(KWMPath.ConfigFile);
}

void KwmExecuteInitScript()
{
    std::string InitFile = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/init";

    struct stat Buffer;
    if(stat(InitFile.c_str(), &Buffer) == 0)
        KwmExecuteThreadedSystemCommand(InitFile);
}

void KwmExecuteFile(std::string File)
{
    std::ifstream FileHandle(KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/" + File);
    if(FileHandle.fail())
    {
        DEBUG("Could not open " << KWMPath.EnvHome << "/" << KWMPath.ConfigFolder << "/" << File
              << ", make sure the file exists." << std::endl);
        return;
    }

    std::string Line;
    std::map<std::string, std::string> Defines;
    while(std::getline(FileHandle, Line))
    {
        if(!Line.empty() && Line[0] != '#')
        {
            KwmSubstitueVariables(Defines, Line);
            if(IsPrefixOfString(Line, "kwmc"))
                KwmInterpretCommand(Line, 0);
            else if(IsPrefixOfString(Line, "exec"))
                KwmExecuteThreadedSystemCommand(Line);
            else if(IsPrefixOfString(Line, "include"))
                KwmExecuteFile(Line);
            else if(IsPrefixOfString(Line, "define"))
                KwmDefineVariable(Defines, Line);
        }
    }

    FileHandle.close();
}

void KwmExecuteSystemCommand(std::string Command)
{
    int ChildPID = fork();
    if(ChildPID == 0)
    {
        DEBUG("Exec: FORK SUCCESS");
        std::vector<std::string> Tokens = SplitString(Command, ' ');
        const char **ExecArgs = new const char*[Tokens.size()+1];
        for(int Index = 0; Index < Tokens.size(); ++Index)
        {
            ExecArgs[Index] = Tokens[Index].c_str();
            DEBUG("Exec argument " << Index << ": " << ExecArgs[Index]);
        }

        ExecArgs[Tokens.size()] = NULL;
        int StatusCode = execvp(ExecArgs[0], (char **)ExecArgs);
        DEBUG("Exec failed with code: " << StatusCode);
        exit(StatusCode);
    }
}

void KwmExecuteThreadedSystemCommand(std::string Command)
{
    std::string *HCommand = new std::string(Command);
    pthread_create(&KWMThread.SystemCommand, NULL, &KwmStartThreadedSystemCommand, HCommand);
}

void * KwmStartThreadedSystemCommand(void *Args)
{
    std::string Command = *((std::string*)Args);
    KwmExecuteSystemCommand(Command);
    delete (std::string*)Args;
    return NULL;
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
    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);

    signal(Signum, SIG_DFL);
    kill(getpid(), Signum);
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

    NSApplicationLoad();
    CFRunLoopRun();
    return 0;
}
