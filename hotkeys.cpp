#include "kwm.h"

static const CGKeyCode kVK_SPECIAL_Å = 0x21;
static const CGKeyCode kVK_SPECIAL_Ø = 0x29;
static const CGKeyCode kVK_SPECIAL_Æ = 0x27;

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
    if(CmdKey && AltKey && CtrlKey)
    {
        switch(Keycode)
        {
            // Toggle Focus-Mode
            case kVK_ANSI_T:
            {
                system("kwmc focus -t toggle");
            } break;
            // Mark Container to split
            case kVK_ANSI_M:
            {
                system("kwmc window -t mark");
            } break;
            // Toggle window floating
            case kVK_ANSI_W:
            {
                system("kwmc window -t float");
            } break;
            // Reapply node container to window 
            case kVK_ANSI_R:
            {
                system("kwmc window -c refresh");
            } break;
            // Restart Kwm
            case kVK_ANSI_Q:
            {
                system("kwmc restart");
            } break;
            // Use Width/Height ratio to determine split mode
            case kVK_ANSI_O:
            {
                system("kwmc screen -s optimal");
            } break;
            // Vertical Split-Mode
            case kVK_ANSI_7:
            {
                system("kwmc screen -s vertical");
            } break;
            // Horizontal Split-Mode
            case kVK_ANSI_Slash:
            {
                system("kwmc screen -s horizontal");
            } break;
            // Toggle Split-Type of existing container
            case kVK_ANSI_S:
            {
                system("kwmc window -c split");
            } break;
            // Resize Panes
            case kVK_ANSI_H:
            {
                system("kwmc screen -m left");
            } break;
            case kVK_ANSI_L:
            {
                system("kwmc screen -m right");
            } break;
            case kVK_ANSI_J:
            {
                system("kwmc screen -m down");
            } break;
            case kVK_ANSI_K:
            {
                system("kwmc screen -m up");
            } break;
            // Toggle Fullscreen / Parent Container
            case kVK_ANSI_F:
            {
                system("kwmc window -t fullscreen");
            } break;
            case kVK_ANSI_P:
            {
                system("kwmc window -t parent");
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
            // Swap focused window with the previous window
            case kVK_ANSI_P:
            {
                system("kwmc window -s prev");
            } break;
            // Swap focused window with the next window
            case kVK_ANSI_N:
            {
                system("kwmc window -s next");
            } break;
            // Shift focus to the previous window
            case kVK_ANSI_H:
            {
                system("kwmc window -f prev");
            } break;
            // Shift focus to the next window
            case kVK_ANSI_L:
            {
                system("kwmc window -f next");
            } break;
            // Rotate window-tree by 180 degrees
            case kVK_ANSI_R:
            {
                system("kwmc screen -r 180");
            } break;
            // Decrease screen gaps
            case kVK_ANSI_X:
            {
                system("kwmc screen -g decrease horizontal");
            } break;
            case kVK_ANSI_Y:
            {
                system("kwmc screen -g decrease vertical");
            } break;
            // Decrease screen padding
            case kVK_LeftArrow:
            {
                system("kwmc screen -p decrease left");
            } break;
            case kVK_RightArrow:
            {
                system("kwmc screen -p decrease right");
            } break;
            case kVK_UpArrow:
            {
                system("kwmc screen -p decrease top");
            } break;
            case kVK_DownArrow:
            {
                system("kwmc screen -p decrease bottom");
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
                system("kwmc screen -m prev");
            } break;
            // Send window to next screen
            case kVK_ANSI_N:
            {
                system("kwmc screen -m next");
            } break;
            // Increase screen gaps
            case kVK_ANSI_X:
            {
                system("kwmc screen -g increase horizontal");
            } break;
            case kVK_ANSI_Y:
            {
                system("kwmc screen -g increase vertical");
            } break;
            // Increase screen padding
            case kVK_LeftArrow:
            {
                system("kwmc screen -p increase left");
            } break;
            case kVK_RightArrow:
            {
                system("kwmc screen -p increase right");
            } break;
            case kVK_UpArrow:
            {
                system("kwmc screen -p increase top");
            } break;
            case kVK_DownArrow:
            {
                system("kwmc screen -p increase bottom");
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

    return Result;
}

extern "C" KWM_HOTKEY_COMMANDS(SystemHotkeyCommands)
{
    bool Result = true;
    if (CmdKey && !CtrlKey && !AltKey)
    {
        switch(Keycode)
        {
            // Spotlight fix
            case kVK_Space:
            {
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
                system("/Applications/iTerm.app/Contents/MacOS/iTerm2 --new-window &");
            } break;
            // YTD - Media Player Controls
            case kVK_ANSI_Z:
            {
                system("ytc prev");
            } break;
            case kVK_ANSI_X:
            {
                system("ytc play");
            } break;
            case kVK_ANSI_C:
            {
                system("ytc next");
            } break;
            case kVK_ANSI_V:
            {
                system("ytc stop");
            } break;
            case kVK_ANSI_A:
            {
                system("ytc volup");
            } break;
            case kVK_ANSI_D:
            {
                system("ytc voldown");
            } break;
            case kVK_LeftArrow:
            {
                system("ytc seekbk");
            } break;
            case kVK_RightArrow:
            {
                system("ytc seekfw");
            } break;
            case kVK_ANSI_Grave:
            {
                system("ytc fav");
            } break;
            default:
            {
                Result = false;
            } break;
        }
    }
    else if(CmdKey && !AltKey && !CtrlKey)
    {
        switch(Keycode)
        {
            // disable retarded hotkey for minimizing an application
            case kVK_ANSI_M:
            {
            } break;
            // disable retarded hotkey for hiding an application
            case kVK_ANSI_H:
            {
            } break;
            // disable cmd+tab window-swap
            case kVK_Tab:
            {
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
