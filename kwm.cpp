#include "kwm.h"

CFMachPortRef EventTap;
kwm_code KWMCode;
export_table ExportTable;
std::string KwmFilePath;
std::string HotkeySOFullFilePath;

std::vector<screen_info> DisplayLst;
std::vector<window_info> WindowLst;
std::vector<int> FloatingWindowLst;
screen_info *Screen;
int CurrentSpace = 0;
int PrevSpace = -1;

ProcessSerialNumber FocusedPSN;
AXUIElementRef FocusedWindowRef;
window_info *FocusedWindow;
int MarkedWindowID = -1;

uint32_t MaxDisplayCount = 5;
uint32_t ActiveDisplaysCount;
CGDirectDisplayID ActiveDisplays[5];

pthread_t BackgroundThread;
pthread_t DaemonThread;

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon)
{
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
            CGEventFlags Flags = CGEventGetFlags(Event);
            bool CmdKey = (Flags & kCGEventFlagMaskCommand) == kCGEventFlagMaskCommand;
            bool AltKey = (Flags & kCGEventFlagMaskAlternate) == kCGEventFlagMaskAlternate;
            bool CtrlKey = (Flags & kCGEventFlagMaskControl) == kCGEventFlagMaskControl;
            CGKeyCode Keycode = (CGKeyCode)CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);

            std::string NewHotkeySOFileTime = KwmGetFileTime(HotkeySOFullFilePath.c_str());
            if(NewHotkeySOFileTime != KWMCode.HotkeySOFileTime)
            {
                DEBUG("Reloading hotkeys.so")
                UnloadKwmCode(&KWMCode);
                KWMCode = LoadKwmCode();
            }

            if(KWMCode.IsValid)
            {
                // Hotkeys specific to Kwms functionality
                if(KWMCode.KWMHotkeyCommands(&ExportTable, CmdKey, CtrlKey, AltKey, Keycode))
                    return NULL;

                // Capture custom hotkeys specified by the user
                if(KWMCode.CustomHotkeyCommands(&ExportTable, CmdKey, CtrlKey, AltKey, Keycode))
                    return NULL;

                // Let system hotkeys pass through as normal
                if(KWMCode.SystemHotkeyCommands(&ExportTable, CmdKey, CtrlKey, AltKey, Keycode))
                    return Event;

                int NewKeycode;
                KWMCode.RemapKeys(Event, &CmdKey, &CtrlKey, &AltKey, Keycode, &NewKeycode);
                if(NewKeycode != -1)
                {
                    if(CmdKey)
                        CGEventSetFlags(Event, kCGEventFlagMaskCommand);
                    if(CtrlKey)
                        CGEventSetFlags(Event, kCGEventFlagMaskControl);
                    if(AltKey)
                        CGEventSetFlags(Event, kCGEventFlagMaskAlternate);

                    CGEventSetIntegerValueField(Event, kCGKeyboardEventKeycode, NewKeycode);
                }
            }

            if(ExportTable.KwmFocusMode == FocusModeAutofocus)
            {
                CGEventSetIntegerValueField(Event, kCGKeyboardEventAutorepeat, 0);
                CGEventPostToPSN(&FocusedPSN, Event);
                return NULL;
            }
        } break;
        case kCGEventMouseMoved:
        {
            if(ExportTable.KwmFocusMode != FocusModeDisabled)
            {
                FocusWindowBelowCursor();
            }
        } break;
    }

    return Event;
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
    stat(File, &attr);

    return ctime(&attr.st_mtime);
}

void BuildExportTable()
{
    ExportTable.KwmFilePath = KwmFilePath;
    ExportTable.KwmFocusMode = FocusModeAutoraise;;
    ExportTable.KwmSplitMode = -1;

    ExportTable.FocusWindowBelowCursor = &FocusWindowBelowCursor;
    ExportTable.ChangeGapOfDisplay = &ChangeGapOfDisplay;
    ExportTable.ChangePaddingOfDisplay = &ChangePaddingOfDisplay;
    ExportTable.ReflectWindowNodeTreeVertically = &ReflectWindowNodeTreeVertically;
    ExportTable.ResizeWindowToContainerSize = &ResizeWindowToContainerSize;;
    ExportTable.MoveContainerSplitter = &MoveContainerSplitter;
    ExportTable.ToggleFocusedWindowFloating = &ToggleFocusedWindowFloating;
    ExportTable.ToggleFocusedWindowFullscreen = &ToggleFocusedWindowFullscreen;
    ExportTable.ToggleFocusedWindowParentContainer = &ToggleFocusedWindowParentContainer;
    ExportTable.SwapFocusedWindowWithNearest = &SwapFocusedWindowWithNearest;
    ExportTable.ShiftWindowFocus = &ShiftWindowFocus;
    ExportTable.CycleFocusedWindowDisplay = &CycleFocusedWindowDisplay;
    ExportTable.MarkWindowContainer = &MarkWindowContainer;
    ExportTable.KwmRestart = &KwmRestart;
}

void KwmRestart()
{
    DEBUG("KWM Restarting..")
    const char **ExecArgs = new const char*[2];
    ExecArgs[0] = "kwm";
    ExecArgs[1] = NULL;
    std::string KwmBinPath = KwmFilePath + "/kwm";
    execv(KwmBinPath.c_str(), (char**)ExecArgs);
}

void * KwmWindowMonitor(void*)
{
    while(1)
    {
        if(ExportTable.KwmFocusMode != FocusModeDisabled)
            UpdateWindowTree();

        usleep(200000);
    }
}

void KwmInit()
{
    if(!CheckPrivileges())
        Fatal("Could not access OSX Accessibility!"); 

    KwmFilePath = getcwd(NULL, 0);
    HotkeySOFullFilePath = KwmFilePath + "/hotkeys.so";
    
    KWMCode = LoadKwmCode();
    BuildExportTable();

    GetActiveDisplays();
    UpdateWindowTree();
    FocusWindowBelowCursor();

    if(KwmStartDaemon())
        pthread_create(&DaemonThread, NULL, &KwmDaemonHandleConnectionBG, NULL);
    else
        DEBUG("Kwm: Could not start daemon..")

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

void Fatal(const std::string &Err)
{
    std::cout << Err << std::endl;
    exit(1);
}

int main(int argc, char **argv)
{
    KwmInit();

    CGEventMask EventMask;
    CFRunLoopSourceRef RunLoopSource;

    EventMask = ((1 << kCGEventKeyDown) | (1 << kCGEventMouseMoved));
    EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, EventMask, CGEventCallback, NULL);

    if(!EventTap || !CGEventTapIsEnabled(EventTap))
        Fatal("ERROR: Could not create event-tap!");

    RunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, EventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), RunLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(EventTap, true);
    CFRunLoopRun();
    return 0;
}
