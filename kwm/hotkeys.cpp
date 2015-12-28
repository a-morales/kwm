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
            // Toggle Focus-Mode
            case kVK_ANSI_T:
            {
                KwmcCommandToExecute = "config focus toggle";
            } break;
            // Mark Container to split
            case kVK_ANSI_M:
            {
                KwmcCommandToExecute = "window -t mark";
            } break;
            // Toggle window floating
            case kVK_ANSI_W:
            {
                KwmcCommandToExecute = "window -t float";
            } break;
            // Reapply node container to window 
            case kVK_ANSI_R:
            {
                KwmcCommandToExecute = "window -c refresh";
            } break;
            // Restart Kwm
            case kVK_ANSI_Q:
            {
                KwmcCommandToExecute = "quit";
            } break;
            // Use Width/Height ratio to determine split mode
            case kVK_ANSI_O:
            {
                KwmcCommandToExecute = "screen -s optimal";
            } break;
            // Vertical Split-Mode
            case kVK_ANSI_7:
            {
                KwmcCommandToExecute = "screen -s vertical";
            } break;
            // Horizontal Split-Mode
            case kVK_ANSI_Slash:
            {
                KwmcCommandToExecute = "screen -s horizontal";
            } break;
            // Toggle Split-Type of existing container
            case kVK_ANSI_S:
            {
                KwmcCommandToExecute = "window -c split";
            } break;
            // Resize Panes
            case kVK_ANSI_H:
            {
                KwmcCommandToExecute = "space -m left";
            } break;
            case kVK_ANSI_L:
            {
                KwmcCommandToExecute = "space -m right";
            } break;
            case kVK_ANSI_J:
            {
                KwmcCommandToExecute = "space -m down";
            } break;
            case kVK_ANSI_K:
            {
                KwmcCommandToExecute = "space -m up";
            } break;
            // Toggle Fullscreen / Parent Container
            case kVK_ANSI_F:
            {
                KwmcCommandToExecute = "window -t fullscreen";
            } break;
            case kVK_ANSI_P:
            {
                KwmcCommandToExecute = "window -t parent";
            } break;
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
            // Toggle space floating/tiling
            case kVK_ANSI_T:
            {
                KwmcCommandToExecute = "space -t toggle";
            } break;
            // Swap focused window with the previous window
            case kVK_ANSI_P:
            {
                KwmcCommandToExecute = "window -s prev";
            } break;
            // Swap focused window with the next window
            case kVK_ANSI_N:
            {
                KwmcCommandToExecute = "window -s next";
            } break;
            // Swap focused window with the marked window
            case kVK_ANSI_M:
            {
                KwmcCommandToExecute = "window -s mark";
            } break;
            // Shift focus to the previous window
            case kVK_ANSI_H:
            {
                KwmcCommandToExecute = "window -f prev";
            } break;
            // Shift focus to the next window
            case kVK_ANSI_L:
            {
                KwmcCommandToExecute = "window -f next";
            } break;
            // Rotate window-tree by 180 degrees
            case kVK_ANSI_R:
            {
                KwmcCommandToExecute = "space -r 180";
            } break;
            // Decrease space gaps
            case kVK_ANSI_X:
            {
                KwmcCommandToExecute = "space -g decrease horizontal";
            } break;
            case kVK_ANSI_Y:
            {
                KwmcCommandToExecute = "space -g decrease vertical";
            } break;
            // Decrease space padding
            case kVK_LeftArrow:
            {
                KwmcCommandToExecute = "space -p decrease left";
            } break;
            case kVK_RightArrow:
            {
                KwmcCommandToExecute = "space -p decrease right";
            } break;
            case kVK_UpArrow:
            {
                KwmcCommandToExecute = "space -p decrease top";
            } break;
            case kVK_DownArrow:
            {
                KwmcCommandToExecute = "space -p decrease bottom";
            } break;
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
            // Send window to previous screen
            case kVK_ANSI_P:
            {
                KwmcCommandToExecute = "screen -m prev";
            } break;
            // Send window to next screen
            case kVK_ANSI_N:
            {
                KwmcCommandToExecute = "screen -m next";
            } break;
            // Send window to screen 0
            case kVK_ANSI_1:
            {
                KwmcCommandToExecute = "screen -m 0";
            } break;
            // Send window to screen 1
            case kVK_ANSI_2:
            {
                KwmcCommandToExecute = "screen -m 1";
            } break;
            // Send window to screen 2
            case kVK_ANSI_3:
            {
                KwmcCommandToExecute = "screen -m 2";
            } break;
            // Increase space gaps
            case kVK_ANSI_X:
            {
                KwmcCommandToExecute = "space -g increase horizontal";
            } break;
            case kVK_ANSI_Y:
            {
                KwmcCommandToExecute = "space -g increase vertical";
            } break;
            // Increase space padding
            case kVK_LeftArrow:
            {
                KwmcCommandToExecute = "space -p increase left";
            } break;
            case kVK_RightArrow:
            {
                KwmcCommandToExecute = "space -p increase right";
            } break;
            case kVK_UpArrow:
            {
                KwmcCommandToExecute = "space -p increase top";
            } break;
            case kVK_DownArrow:
            {
                KwmcCommandToExecute = "space -p increase bottom";
            } break;
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
    if(CmdKey && AltKey && CtrlKey)
    {
        switch(Keycode)
        {
            // Open New iTerm Window
            case kVK_Return:
            {
                system("open -na /Applications/iTerm.app");
            } break;
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
