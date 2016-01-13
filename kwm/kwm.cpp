#include "kwm.h"

const std::string KwmCurrentVersion = "Kwm Version 1.0.5";
const std::string PlistFile = "com.koekeishiya.kwm.plist";

CFMachPortRef EventTap;
kwm_path KWMPath = {};
kwm_code KWMCode = {};
kwm_screen KWMScreen = {};
kwm_toggles KWMToggles = {};
kwm_prefix KWMPrefix = {};
kwm_focus KWMFocus = {};
std::vector<hotkey> KwmHotkeys;

std::map<unsigned int, screen_info> DisplayMap;
std::vector<window_info> WindowLst;
std::vector<int> FloatingWindowLst;
std::vector<std::string> FloatingAppLst;
std::map<std::string, int> CapturedAppLst;
std::map<std::string, std::vector<CFTypeRef> > AllowedWindowRoles;

space_tiling_option KwmSpaceMode;
focus_option KwmFocusMode;
cycle_focus_option KwmCycleMode;

pthread_t BackgroundThread;
pthread_t DaemonThread;
pthread_mutex_t BackgroundLock;

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon)
{
    pthread_mutex_lock(&BackgroundLock);

    switch(Type)
    {
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
        {
            DEBUG("Restarting Event Tap")
            CGEventTapEnable(EventTap, true);
        } break;
        case kCGEventKeyDown:
        {
            if(KWMToggles.UseBuiltinHotkeys && KwmMainHotkeyTrigger(&Event))
            {
                    pthread_mutex_unlock(&BackgroundLock);
                    return NULL;
            }

            if(KwmFocusMode == FocusModeAutofocus)
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&KWMFocus.PSN, Event);
                pthread_mutex_unlock(&BackgroundLock);
                return NULL;
            }
        } break;
        case kCGEventKeyUp:
        {
            if(KwmFocusMode == FocusModeAutofocus)
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&KWMFocus.PSN, Event);
                pthread_mutex_unlock(&BackgroundLock);
                return NULL;
            }
        } break;
        case kCGEventMouseMoved:
        {
            if(KwmFocusMode != FocusModeDisabled)
                FocusWindowBelowCursor();
        } break;
        case kCGEventLeftMouseDown:
        {
            DEBUG("Left mouse button was pressed")
            FocusWindowBelowCursor();
            if(KWMToggles.EnableDragAndDrop && IsCursorInsideFocusedWindow())
               KWMToggles.WindowDragInProgress = true;
        } break;
        case kCGEventLeftMouseUp:
        {
            if(KWMToggles.EnableDragAndDrop && KWMToggles.WindowDragInProgress)
            {
                if(!IsCursorInsideFocusedWindow())
                    ToggleFocusedWindowFloating();

                KWMToggles.WindowDragInProgress = false;
            }

            DEBUG("Left mouse button was released")
        } break;
    }

    pthread_mutex_unlock(&BackgroundLock);
    return Event;
}

bool KwmRunLiveCodeHotkeySystem(CGEventRef *Event, modifiers *Mod, CGKeyCode Keycode)
{
    std::string NewHotkeySOFileTime = KwmGetFileTime(KWMPath.HotkeySOFullPath.c_str());
    if(NewHotkeySOFileTime != "file not found" &&
       NewHotkeySOFileTime != KWMCode.HotkeySOFileTime)
    {
        DEBUG("Reloading hotkeys.so")
        UnloadKwmCode(&KWMCode);
        KWMCode = LoadKwmCode();
    }

    if(KWMCode.IsValid)
    {
        // Capture custom hotkeys specified in hotkeys.cpp
        if(KWMCode.KWMHotkeyCommands(*Mod, Keycode))
            return true;

        // Check if key should be remapped
        int NewKeycode;
        KWMCode.RemapKeys(Mod, Keycode, &NewKeycode);
        if(NewKeycode != -1)
        {
            CGEventSetFlags(*Event, 0);

            if(Mod->CmdKey)
                CGEventSetFlags(*Event, kCGEventFlagMaskCommand);

            if(Mod->AltKey)
                CGEventSetFlags(*Event, kCGEventFlagMaskAlternate);

            if(Mod->CtrlKey)
                CGEventSetFlags(*Event, kCGEventFlagMaskControl);

            if(Mod->ShiftKey)
                CGEventSetFlags(*Event, kCGEventFlagMaskShift);

            CGEventSetIntegerValueField(*Event, kCGKeyboardEventKeycode, NewKeycode);
        }
    }

    return false;
}

kwm_code LoadKwmCode()
{
    kwm_code Code = {};

    Code.HotkeySOFileTime = KwmGetFileTime(KWMPath.HotkeySOFullPath.c_str());
    Code.KwmHotkeySO = dlopen(KWMPath.HotkeySOFullPath.c_str(),  RTLD_LAZY);
    if(Code.KwmHotkeySO)
    {
        Code.KWMHotkeyCommands = (kwm_hotkey_commands*) dlsym(Code.KwmHotkeySO, "KWMHotkeyCommands");
        Code.RemapKeys = (kwm_key_remap*) dlsym(Code.KwmHotkeySO, "RemapKeys");
    }
    else
    {
        DEBUG("LoadKwmCode() Could not open '" << KWMPath.HotkeySOFullPath << "'")
    }

    Code.IsValid = (Code.KWMHotkeyCommands && Code.RemapKeys);
    return Code;
}

void UnloadKwmCode(kwm_code *Code)
{
    if(Code->KwmHotkeySO)
        dlclose(Code->KwmHotkeySO);

    Code->HotkeySOFileTime = "";
    Code->KWMHotkeyCommands = 0;
    Code->RemapKeys = 0;
    Code->IsValid = 0;
}

std::string KwmGetFileTime(const char *File)
{
    struct stat attr;

    int Result = stat(File, &attr);
    if(Result == -1)
        return "file not found";

    return ctime(&attr.st_mtime);
}

void KwmQuit()
{
    exit(0);
}

void * KwmWindowMonitor(void*)
{
    while(1)
    {
        pthread_mutex_lock(&BackgroundLock);
        UpdateWindowTree();
        pthread_mutex_unlock(&BackgroundLock);
        usleep(200000);
    }
}

bool IsPrefixOfString(std::string &Line, std::string Prefix)
{
    bool Result = false;

    if(Line.substr(0, Prefix.size()) == Prefix)
    {
        Line = Line.substr(Prefix.size()+1);
        Result = true;
    }

    return Result;
}

void KwmReloadConfig()
{
    KwmClearSettings();
    KwmExecuteConfig();
}

void KwmClearSettings()
{
    std::map<std::string, std::vector<CFTypeRef> >::iterator It;
    for(It = AllowedWindowRoles.begin(); It != AllowedWindowRoles.end(); ++It)
    {
        std::vector<CFTypeRef> &WindowRoles = It->second;
        for(std::size_t RoleIndex = 0; RoleIndex < WindowRoles.size(); ++RoleIndex)
            CFRelease(WindowRoles[RoleIndex]);

        WindowRoles.clear();
    }

    FloatingAppLst.clear();
    KwmHotkeys.clear();
    KWMPrefix.Enabled = false;
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

    std::string PlistFullPath = KWMPath.FilePath + "/" + PlistFile;
    std::string SymlinkFullPath = KWMPath.EnvHome + "/Library/LaunchAgents/" + PlistFile;

    std::ifstream TemplateFD(KWMPath.FilePath + "/kwm_template.plist");
    if(TemplateFD.fail())
        return;

    std::ofstream OutFD(PlistFullPath);
    if(OutFD.fail())
        return;

    std::string Line;
    std::vector<std::string> PlistContents;
    while(std::getline(TemplateFD, Line))
        PlistContents.push_back(Line);

    DEBUG("AddKwmToLaunchd() Creating file: " << PlistFullPath)
    for(std::size_t LineNumber = 0; LineNumber < PlistContents.size(); ++LineNumber)
    {
        if(LineNumber == 8)
            OutFD << "    <string>" + KWMPath.FilePath + "/kwm</string>" << std::endl;
        else
            OutFD << PlistContents[LineNumber] << std::endl;
    }

    TemplateFD.close();
    OutFD.close();

    std::string PerformSymlink = "mv " + PlistFullPath + " " + SymlinkFullPath;
    system(PerformSymlink.c_str());

    DEBUG("AddKwmToLaunchd() Moved plist to: " << SymlinkFullPath)
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

    if (pthread_mutex_init(&BackgroundLock, NULL) != 0)
        Fatal("Could not create mutex!");

    if(KwmStartDaemon())
        pthread_create(&DaemonThread, NULL, &KwmDaemonHandleConnectionBG, NULL);
    else
        Fatal("Kwm: Could not start daemon..");

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
    KWMToggles.UseContextMenuFix = true;
    KWMToggles.UseMouseFollowsFocus = true;
    KWMToggles.WindowDragInProgress = false;

    KwmSpaceMode = SpaceModeBSP;
    KwmFocusMode = FocusModeAutoraise;
    KwmCycleMode = CycleModeScreen;

    KWMPrefix.Enabled = false;
    KWMPrefix.Active = false;
    KWMPrefix.Timeout = 0.75;

    KWMPath.ConfigFile = "kwmrc";
    if(GetKwmFilePath())
        KWMCode = LoadKwmCode();

    KwmExecuteConfig();
    GetActiveDisplays();

    pthread_create(&BackgroundThread, NULL, &KwmWindowMonitor, NULL);
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
    CGEventMask EventMask;

    CFRunLoopSourceRef RunLoopSource;

    EventMask = ((1 << kCGEventKeyDown) |
                 (1 << kCGEventKeyUp) |
                 (1 << kCGEventMouseMoved) |
                 (1 << kCGEventLeftMouseDown) |
                 (1 << kCGEventLeftMouseUp));

    EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, EventMask, CGEventCallback, NULL);

    if(!EventTap || !CGEventTapIsEnabled(EventTap))
        Fatal("ERROR: Could not create event-tap!");

    RunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, EventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), RunLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(EventTap, true);
    NSApplicationLoad();
    CFRunLoopRun();

    return 0;
}
