#include "kwm.h"

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

std::string KwmcFilePath;
std::string KwmcCommandToExecute;

// This function is run once every time
// hotkeys.so is recompiled.
//
// The path to Kwmc is set in KwmcFilePath
// To bind a Kwmc command, simply set the
// value of KwmcCommandToExecute.
//
// e.g KwmcCommandToExecute = "window -f prev"
void GetKwmcFilePath()
{
    if(!KwmcFilePath.empty())
        return;

    char PathBuf[PROC_PIDPATHINFO_MAXSIZE];
    pid_t Pid = getpid();
    std::string Path;

    int Ret = proc_pidpath(Pid, PathBuf, sizeof(PathBuf));
    if (Ret > 0)
    {
        Path = PathBuf;

        std::size_t Split = Path.find_last_of("/\\");
        Path = Path.substr(0, Split);
        KwmcFilePath = Path + "/kwmc";
        std::cout << "GetKwmcFilePath()" << std::endl;
    }
}

// Takes a command and executes it using popen,
// reads from stdout and returns the value
std::string GetOutputAndExec(std::string Cmd)
{
    std::shared_ptr<FILE> Pipe(popen(Cmd.c_str(), "r"), pclose);
    if (!Pipe)
        return "ERROR";

    char Buffer[128];
    std::string Result = "";

    while (!feof(Pipe.get()))
    {
        if (fgets(Buffer, 128, Pipe.get()) != NULL)
            Result += Buffer;
    }

    return Result;
}

extern "C" KWM_KEY_REMAP(RemapKeys)
{
    *Result = -1;

    /*

    if(*CmdKey && *AltKey && *CtrlKey)
    {
    }

    switch(Keycode)
    {
        case kVK_ANSI_A:
        {
            *Result = kVK_ANSI_S;
        } break;
    }

    Change wether a modifier flag should be set or not.
    If a modifier is not changed, the value already set for
    the event will be used.

    *CmdKey = true;
    *AltKey = true;
    *CtrlKey = true;
    *ShiftKey = true;

    */
}

extern "C" KWM_HOTKEY_COMMANDS(KWMHotkeyCommands)
{
    bool Result = true;
    GetKwmcFilePath();

    if(CmdKey && AltKey && CtrlKey)
    {
        switch(Keycode)
        {
            default:
            {
                Result = false;
            } break;
        }
    }
    else if(CmdKey && AltKey && !CtrlKey)
    {
        switch(Keycode)
        {
            default:
            {
                Result = false;
            } break;
        }
    }
    else if(!CmdKey && CtrlKey && AltKey)
    {
        switch(Keycode)
        {
            default:
            {
                Result = false;
            };
        }
    }
    else
    {
        Result = false;
    }

    if(Result)
    {
        std::string SysArg = KwmcFilePath + " " + KwmcCommandToExecute;
        system(SysArg.c_str());
        KwmcCommandToExecute = "";
    }

    return Result;
}

extern "C" KWM_HOTKEY_COMMANDS(SystemHotkeyCommands)
{
    bool Result = true;
    if (CmdKey && !CtrlKey && !AltKey)
    {
        switch(Keycode)
        {
            default:
            {
                Result = false;
            } break;
        }
    }
    else
    {
        Result = false;
    }

    return Result;
}

extern "C" KWM_HOTKEY_COMMANDS(CustomHotkeyCommands)
{
    bool Result = true;
    GetKwmcFilePath();

    if(CmdKey && AltKey && CtrlKey)
    {
        switch(Keycode)
        {
            /* This demonstrates how a hotkey may do different things
             * depending on what the focused window is.
            case kVK_ANSI_N:
            {
                std::string Focused = GetOutputAndExec(KwmcFilePath + " focused");
                std::stringstream Stream(Focused);

                std::string Owner;
                std::getline(Stream, Owner, '-');
                Owner.erase(Owner.end()-1);

                if(Owner == "iTerm2")
                    KwmcCommandToExecute = "write This is written inside iTerm";
                else if(Owner == "Google Chrome")
                    KwmcCommandToExecute = "write This is written inside Google Chrome";
                else
                    KwmcCommandToExecute = "write This was written in " + Owner + "!";
            } break;
            */
            default:
            {
                Result = false;
            } break;
        }
    }
    else
    {
        Result = false;
    }

    if(Result && !KwmcCommandToExecute.empty())
    {
        std::string SysArg = KwmcFilePath + " " + KwmcCommandToExecute;
        system(SysArg.c_str());
        KwmcCommandToExecute = "";
    }

    return Result;
}
