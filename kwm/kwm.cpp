#include "kwm.h"
#include "helpers.h"
#include "daemon.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "keys.h"
#include "interpreter.h"
#include "border.h"

const std::string KwmCurrentVersion = "Kwm Version 1.1.0";
const std::string PlistFile = "com.koekeishiya.kwm.plist";

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
kwm_border PrefixBorder = {};

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon)
{
    pthread_mutex_lock(&KWMThread.Lock);

    switch(Type)
    {
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
        {
            DEBUG("Restarting Event Tap")
            CGEventTapEnable(KWMMach.EventTap, true);
        } break;
        case kCGEventKeyDown:
        {
            CheckPrefixTimeout();
            if(KWMToggles.UseBuiltinHotkeys && KwmMainHotkeyTrigger(&Event))
            {
                    pthread_mutex_unlock(&KWMThread.Lock);
                    return NULL;
            }

            if(KWMMode.Focus == FocusModeAutofocus &&
               !IsActiveSpaceFloating())
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&KWMFocus.PSN, Event);
                pthread_mutex_unlock(&KWMThread.Lock);
                return NULL;
            }
        } break;
        case kCGEventKeyUp:
        {
            CheckPrefixTimeout();
            if(KWMMode.Focus == FocusModeAutofocus &&
               !IsActiveSpaceFloating())
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&KWMFocus.PSN, Event);
                pthread_mutex_unlock(&KWMThread.Lock);
                return NULL;
            }
        } break;
        case kCGEventMouseMoved:
        {
            if(KWMMode.Focus != FocusModeDisabled &&
               KWMMode.Focus != FocusModeStandby &&
               !IsActiveSpaceFloating())
                FocusWindowBelowCursor();
        } break;
        case kCGEventLeftMouseDown:
        {
            DEBUG("Left mouse button was pressed")
            if(!IsActiveSpaceFloating())
            {
                FocusWindowBelowCursor();
                if(KWMToggles.EnableDragAndDrop)
                    KWMToggles.DragInProgress = true;
            }
        } break;
        case kCGEventLeftMouseUp:
        {
            DEBUG("Left mouse button was released")
            if(!IsActiveSpaceFloating())
            {
                if(KWMToggles.EnableDragAndDrop && KWMToggles.DragInProgress)
                    KWMToggles.DragInProgress = false;

                if(KWMFocus.Window && FocusedBorder.Enabled)
                {
                    if(IsWindowFloating(KWMFocus.Window->WID, NULL))
                        UpdateBorder("focused");
                }
            }
        } break;
    }

    pthread_mutex_unlock(&KWMThread.Lock);
    return Event;
}

void KwmQuit()
{
    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);
    CloseBorder(&PrefixBorder);

    exit(0);
}

void * KwmWindowMonitor(void*)
{
    while(1)
    {
        pthread_mutex_lock(&KWMThread.Lock);
        UpdateWindowTree();
        pthread_mutex_unlock(&KWMThread.Lock);
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
    std::map<std::string, std::vector<CFTypeRef> >::iterator It;
    for(It = KWMTiling.AllowedWindowRoles.begin(); It != KWMTiling.AllowedWindowRoles.end(); ++It)
    {
        std::vector<CFTypeRef> &WindowRoles = It->second;
        for(std::size_t RoleIndex = 0; RoleIndex < WindowRoles.size(); ++RoleIndex)
            CFRelease(WindowRoles[RoleIndex]);

        WindowRoles.clear();
    }

    KWMTiling.FloatingAppLst.clear();
    KWMHotkeys.List.clear();
    KWMHotkeys.Prefix.Enabled = false;
}

void KwmExecuteConfig()
{
    char *HomeP = std::getenv("HOME");
    if(!HomeP)
    {
        DEBUG("Failed to get environment variable 'HOME'")
        return;
    }

    KWMPath.EnvHome = HomeP;
    KWMPath.ConfigFolder = ".kwm";
    std::ifstream ConfigFD(KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/" + KWMPath.ConfigFile);
    if(ConfigFD.fail())
    {
        DEBUG("Could not open " << KWMPath.EnvHome << "/" << KWMPath.ConfigFolder << "/" << KWMPath.ConfigFile
              << ", make sure the file exists." << std::endl)
        return;
    }

    std::string Line;
    while(std::getline(ConfigFD, Line))
    {
        if(!Line.empty() && Line[0] != '#')
        {
            if(IsPrefixOfString(Line, "kwmc"))
                KwmInterpretCommand(Line, 0);
            else if(IsPrefixOfString(Line, "sys"))
                system(Line.c_str());
        }
    }

    ConfigFD.close();
}

bool IsKwmAlreadyAddedToLaunchd()
{
    std::string SymlinkFullPath = KWMPath.EnvHome + "/Library/LaunchAgents/" + PlistFile;

    struct stat attr;
    int Result = stat(SymlinkFullPath.c_str(), &attr);
    if(Result == -1)
        return false;

    return true;
}

void AddKwmToLaunchd()
{
    if(IsKwmAlreadyAddedToLaunchd())
        return;

    std::string SymlinkFullPath = KWMPath.EnvHome + "/Library/LaunchAgents/" + PlistFile;

    std::ifstream TemplateFD(KWMPath.FilePath + "/kwm_template.plist");
    if(TemplateFD.fail())
        return;

    std::ofstream OutFD(SymlinkFullPath);
    if(OutFD.fail())
        return;

    std::string Line;
    std::vector<std::string> PlistContents;
    while(std::getline(TemplateFD, Line))
        PlistContents.push_back(Line);

    DEBUG("AddKwmToLaunchd() Creating file: " << SymlinkFullPath)
    for(std::size_t LineNumber = 0; LineNumber < PlistContents.size(); ++LineNumber)
    {
        if(LineNumber == 8)
            OutFD << "    <string>" + KWMPath.FilePath + "/kwm</string>" << std::endl;
        else
            OutFD << PlistContents[LineNumber] << std::endl;
    }

    TemplateFD.close();
    OutFD.close();
}

void RemoveKwmFromLaunchd()
{
    if(!IsKwmAlreadyAddedToLaunchd())
        return;

    std::string SymlinkFullPath = KWMPath.EnvHome + "/Library/LaunchAgents/" + PlistFile;
    std::string RemoveSymlink = "rm " + SymlinkFullPath;

    system(RemoveSymlink.c_str());
    DEBUG("RemoveKwmFromLaunchd() Removing file: " << SymlinkFullPath)
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
        KWMPath.HotkeySOFullPath = KWMPath.FilePath + "/hotkeys.so";
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

    KWMScreen.SplitRatio = 0.5;
    KWMScreen.SplitMode = -1;
    KWMScreen.MarkedWindow = -1;
    KWMScreen.OldScreenID = -1;
    KWMScreen.PrevSpace = -1;
    KWMScreen.DefaultOffset = CreateDefaultScreenOffset();
    KWMScreen.MaxCount = 5;
    KWMScreen.ActiveCount = 0;
    KWMScreen.UpdateSpace = true;

    KWMToggles.EnableTilingMode = true;
    KWMToggles.UseBuiltinHotkeys = true;
    KWMToggles.EnableDragAndDrop = true;
    KWMToggles.UseMouseFollowsFocus = true;

    KWMMode.Space = SpaceModeBSP;
    KWMMode.Focus = FocusModeAutoraise;
    KWMMode.Cycle = CycleModeScreen;

    KWMHotkeys.Prefix.Enabled = false;
    KWMHotkeys.Prefix.Global = false;
    KWMHotkeys.Prefix.Active = false;
    KWMHotkeys.Prefix.Timeout = 0.75;

    KWMPath.ConfigFile = "kwmrc";
    GetKwmFilePath();

    KwmExecuteConfig();
    GetActiveDisplays();

    pthread_create(&KWMThread.WindowMonitor, NULL, &KwmWindowMonitor, NULL);
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
    CloseBorder(&PrefixBorder);

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
                         (1 << kCGEventMouseMoved) |
                         (1 << kCGEventLeftMouseDown) |
                         (1 << kCGEventLeftMouseUp));

    KWMMach.EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, KWMMach.EventMask, CGEventCallback, NULL);
    if(!KWMMach.EventTap || !CGEventTapIsEnabled(KWMMach.EventTap))
        Fatal("ERROR: Could not create event-tap!");

    CFRunLoopAddSource(CFRunLoopGetCurrent(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, KWMMach.EventTap, 0),
                       kCFRunLoopCommonModes);

    CGEventTapEnable(KWMMach.EventTap, true);
    NSApplicationLoad();
    CFRunLoopRun();
    return 0;
}
