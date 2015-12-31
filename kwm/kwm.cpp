#include "kwm.h"

const std::string KwmCurrentVersion = "Kwm Version 1.0.0 RC";
const std::string PlistFile = "com.koekeishiya.kwm.plist";

CFMachPortRef EventTap;

kwm_code KWMCode;
std::string ENV_HOME;
std::string KwmFilePath;
std::string HotkeySOFullFilePath;
bool KwmUseBSPTilingMode;
bool KwmUseBuiltinHotkeys;
bool KwmEnableDragAndDrop;
bool KwmUseContextMenuFix;

uint32_t MaxDisplayCount = 5;
uint32_t ActiveDisplaysCount;
CGDirectDisplayID ActiveDisplays[5];

screen_info *Screen;
int DefaultPaddingTop = 40, DefaultPaddingBottom = 20;
int DefaultPaddingLeft = 20, DefaultPaddingRight = 20;
int DefaultGapVertical = 10, DefaultGapHorizontal = 10;

std::map<unsigned int, screen_info> DisplayMap;
std::vector<window_info> WindowLst;
std::vector<int> FloatingSpaceLst;
std::vector<std::string> FloatingAppLst;
std::vector<int> FloatingWindowLst;

ProcessSerialNumber FocusedPSN;
window_info *FocusedWindow;
focus_option KwmFocusMode;
int KwmSplitMode = -1;
int MarkedWindowID = -1;
bool IsWindowDragInProgress = false;

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
            if(KwmUseBuiltinHotkeys)
            {
                CGEventFlags Flags = CGEventGetFlags(Event);
                bool CmdKey = (Flags & kCGEventFlagMaskCommand) == kCGEventFlagMaskCommand;
                bool AltKey = (Flags & kCGEventFlagMaskAlternate) == kCGEventFlagMaskAlternate;
                bool CtrlKey = (Flags & kCGEventFlagMaskControl) == kCGEventFlagMaskControl;
                bool ShiftKey = (Flags & kCGEventFlagMaskShift) == kCGEventFlagMaskShift;

                CGKeyCode Keycode = (CGKeyCode)CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);

                std::string NewHotkeySOFileTime = KwmGetFileTime(HotkeySOFullFilePath.c_str());
                if(NewHotkeySOFileTime != "file not found" &&
                   NewHotkeySOFileTime != KWMCode.HotkeySOFileTime)
                {
                    DEBUG("Reloading hotkeys.so")
                    UnloadKwmCode(&KWMCode);
                    KWMCode = LoadKwmCode();
                }

                if(KWMCode.IsValid)
                {
                    // Hotkeys specific to Kwms functionality
                    if(KWMCode.KWMHotkeyCommands(CmdKey, CtrlKey, AltKey, ShiftKey, Keycode))
                    {
                        pthread_mutex_unlock(&BackgroundLock);
                        return NULL;
                    }

                    // Capture custom hotkeys specified by the user
                    if(KWMCode.CustomHotkeyCommands(CmdKey, CtrlKey, AltKey, ShiftKey, Keycode))
                    {
                        pthread_mutex_unlock(&BackgroundLock);
                        return NULL;
                    }

                    // Let system hotkeys pass through as normal
                    if(KWMCode.SystemHotkeyCommands(CmdKey, CtrlKey, AltKey, ShiftKey, Keycode))
                    {
                        pthread_mutex_unlock(&BackgroundLock);
                        return Event;
                    }

                    int NewKeycode;
                    KWMCode.RemapKeys(Event, &CmdKey, &CtrlKey, &AltKey, &ShiftKey, Keycode, &NewKeycode);
                    if(NewKeycode != -1)
                    {
                        CGEventSetFlags(Event, 0);

                        if(CmdKey)
                            CGEventSetFlags(Event, kCGEventFlagMaskCommand);

                        if(AltKey)
                            CGEventSetFlags(Event, kCGEventFlagMaskAlternate);

                        if(CtrlKey)
                            CGEventSetFlags(Event, kCGEventFlagMaskControl);

                        if(ShiftKey)
                            CGEventSetFlags(Event, kCGEventFlagMaskShift);

                        CGEventSetIntegerValueField(Event, kCGKeyboardEventKeycode, NewKeycode);
                    }
                }
            }

            if(KwmFocusMode == FocusModeAutofocus)
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&FocusedPSN, Event);
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
            if(KwmEnableDragAndDrop && IsCursorInsideFocusedWindow())
               IsWindowDragInProgress = true;
        } break;
        case kCGEventLeftMouseUp:
        {
            if(KwmEnableDragAndDrop && IsWindowDragInProgress)
            {
                if(!IsCursorInsideFocusedWindow())
                    ToggleFocusedWindowFloating();

                IsWindowDragInProgress = false;
            }

            DEBUG("Left mouse button was released")
        } break;
    }

    pthread_mutex_unlock(&BackgroundLock);
    return Event;
}

void KwmEmitKeystrokes(std::string Text)
{
    CFStringRef TextRef = CFStringCreateWithCString(NULL, Text.c_str(), kCFStringEncodingMacRoman);
    CGEventRef EventKeyDown = CGEventCreateKeyboardEvent(NULL, 0, true);
    CGEventRef EventKeyUp = CGEventCreateKeyboardEvent(NULL, 0, false);

    UniChar OutputBuffer;
    for (int i = 0; i < Text.size(); ++i) {
        CFStringGetCharacters(TextRef, CFRangeMake(i, 1), &OutputBuffer);

        CGEventSetFlags(EventKeyDown, 0);
        CGEventKeyboardSetUnicodeString(EventKeyDown, 1, &OutputBuffer);
        CGEventPostToPSN(&FocusedPSN, EventKeyDown);

        CGEventSetFlags(EventKeyUp, 0);
        CGEventKeyboardSetUnicodeString(EventKeyUp, 1, &OutputBuffer);
        CGEventPostToPSN(&FocusedPSN, EventKeyUp);
    }

    CFRelease(EventKeyUp);
    CFRelease(EventKeyDown);
    CFRelease(TextRef);
}

kwm_code LoadKwmCode()
{
    kwm_code Code = {};

    Code.HotkeySOFileTime = KwmGetFileTime(HotkeySOFullFilePath.c_str());
    Code.KwmHotkeySO = dlopen(HotkeySOFullFilePath.c_str(),  RTLD_LAZY);
    if(Code.KwmHotkeySO)
    {
        Code.KWMHotkeyCommands = (kwm_hotkey_commands*) dlsym(Code.KwmHotkeySO, "KWMHotkeyCommands");
        Code.SystemHotkeyCommands = (kwm_hotkey_commands*) dlsym(Code.KwmHotkeySO, "SystemHotkeyCommands");
        Code.CustomHotkeyCommands = (kwm_hotkey_commands*) dlsym(Code.KwmHotkeySO, "CustomHotkeyCommands");
        Code.RemapKeys = (kwm_key_remap*) dlsym(Code.KwmHotkeySO, "RemapKeys");
    }
    else
    {
        DEBUG("LoadKwmCode() Could not open '" << HotkeySOFullFilePath << "'")
    }

    Code.IsValid = (Code.KWMHotkeyCommands && Code.SystemHotkeyCommands && Code.CustomHotkeyCommands && Code.RemapKeys);
    return Code;
}

void UnloadKwmCode(kwm_code *Code)
{
    if(Code->KwmHotkeySO)
        dlclose(Code->KwmHotkeySO);

    Code->HotkeySOFileTime = "";
    Code->KWMHotkeyCommands = 0;
    Code->SystemHotkeyCommands = 0;
    Code->CustomHotkeyCommands = 0;
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

void KwmExecuteConfig()
{
    char *HomeP = std::getenv("HOME");
    if(!HomeP)
    {
        DEBUG("Failed to get environment variable 'HOME'")
        return;
    }

    ENV_HOME = HomeP;
    std::string KWM_CONFIG_FILE = ".kwmrc";

    std::ifstream ConfigFD(ENV_HOME + "/" + KWM_CONFIG_FILE);
    if(ConfigFD.fail())
    {
        DEBUG("Could not open " << ENV_HOME << "/" << KWM_CONFIG_FILE
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
    std::string SymlinkFullPath = ENV_HOME + "/Library/LaunchAgents/" + PlistFile;

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

    std::string PlistFullPath = KwmFilePath + "/" + PlistFile;
    std::string SymlinkFullPath = ENV_HOME + "/Library/LaunchAgents/" + PlistFile;

    std::ifstream TemplateFD(KwmFilePath + "/kwm_template.plist");
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
    for(int LineNumber = 0; LineNumber < PlistContents.size(); ++LineNumber)
    {
        if(LineNumber == 8)
            OutFD << "    <string>" + KwmFilePath + "/kwm</string>" << std::endl;
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

    std::string SymlinkFullPath = ENV_HOME + "/Library/LaunchAgents/" + PlistFile;
    std::string RemoveSymlink = "rm " + SymlinkFullPath;

    system(RemoveSymlink.c_str());
    DEBUG("RemoveKwmFromLaunchd() Removing file: " << SymlinkFullPath)
}

void GetKwmFilePath()
{
    char PathBuf[PROC_PIDPATHINFO_MAXSIZE];
    pid_t Pid = getpid();
    int Ret = proc_pidpath(Pid, PathBuf, sizeof(PathBuf));
    if (Ret > 0)
    {
        KwmFilePath = PathBuf;

        std::size_t Split = KwmFilePath.find_last_of("/\\");
        KwmFilePath = KwmFilePath.substr(0, Split);
        HotkeySOFullFilePath = KwmFilePath + "/hotkeys.so";
    }
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

    KwmUseBSPTilingMode = true;
    KwmUseBuiltinHotkeys = true;
    KwmEnableDragAndDrop = true;
    KwmUseContextMenuFix = true;
    KwmFocusMode = FocusModeAutoraise;

    GetKwmFilePath();
    KwmExecuteConfig();
    KWMCode = LoadKwmCode();
    GetActiveDisplays();

    pthread_create(&BackgroundThread, NULL, &KwmWindowMonitor, NULL);
}

bool CheckPrivileges()
{
    const void * Keys[] = { kAXTrustedCheckOptionPrompt };
    const void * Values[] = { kCFBooleanTrue };

    CFDictionaryRef Options;
    Options = CFDictionaryCreate(kCFAllocatorDefault, 
            Keys, Values, sizeof(Keys) / sizeof(*Keys),
            &kCFCopyStringDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

    return AXIsProcessTrustedWithOptions(Options);
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
