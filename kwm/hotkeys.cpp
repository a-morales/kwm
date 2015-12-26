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
                system("kwmc quit");
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
                system("kwmc space -m left");
            } break;
            case kVK_ANSI_L:
            {
                system("kwmc space -m right");
            } break;
            case kVK_ANSI_J:
            {
                system("kwmc space -m down");
            } break;
            case kVK_ANSI_K:
            {
                system("kwmc space -m up");
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
            // Toggle space floating/tiling
            case kVK_ANSI_T:
            {
                system("kwmc space -t toggle");
            } break;
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
            // Swap focused window with the marked window
            case kVK_ANSI_M:
            {
                system("kwmc window -s mark");
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
                system("kwmc space -r 180");
            } break;
            // Decrease space gaps
            case kVK_ANSI_X:
            {
                system("kwmc space -g decrease horizontal");
            } break;
            case kVK_ANSI_Y:
            {
                system("kwmc space -g decrease vertical");
            } break;
            // Decrease space padding
            case kVK_LeftArrow:
            {
                system("kwmc space -p decrease left");
            } break;
            case kVK_RightArrow:
            {
                system("kwmc space -p decrease right");
            } break;
            case kVK_UpArrow:
            {
                system("kwmc space -p decrease top");
            } break;
            case kVK_DownArrow:
            {
                system("kwmc space -p decrease bottom");
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
            // Send window to screen 0
            case kVK_ANSI_1:
            {
                system("kwmc screen -m 0");
            } break;
            // Send window to screen 1
            case kVK_ANSI_2:
            {
                system("kwmc screen -m 1");
            } break;
            // Send window to screen 2
            case kVK_ANSI_3:
            {
                system("kwmc screen -m 2");
            } break;
            // Increase space gaps
            case kVK_ANSI_X:
            {
                system("kwmc space -g increase horizontal");
            } break;
            case kVK_ANSI_Y:
            {
                system("kwmc space -g increase vertical");
            } break;
            // Increase space padding
            case kVK_LeftArrow:
            {
                system("kwmc space -p increase left");
            } break;
            case kVK_RightArrow:
            {
                system("kwmc space -p increase right");
            } break;
            case kVK_UpArrow:
            {
                system("kwmc space -p increase top");
            } break;
            case kVK_DownArrow:
            {
                system("kwmc space -p increase bottom");
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
