#include "help.h"

#include <iostream>

void ShowUsage()
{
    std::cout <<
        "Usage: Kwmc <command>\n"
        "\n"
        "Command:\n"
        "   quit                                                      Quit Kwm\n"
        "   help                                                      Display this screen\n"
        "   config                                                    Set configuration for Kwm\n"
        "   window                                                    Manipulate current window\n"
        "   mark                                                      Manage marked window\n"
        "   tree                                                      Manipulate current window-tree\n"
        "   space                                                     Manipulate current space\n"
        "   screen                                                    Manipulate current screen\n"
        "   read                                                      Retrieve current Kwm settings\n"
        "   write sentence                                            Automatically emit keystrokes to the focused window\n"
        "   press mod+mod+mod-key                                     Send a key press with the specified modifiers\n"
        "   bind prefix+mod+mod+mod-key command                       Create a global hotkey (use `sys` prefix for non kwmc command)\n"
        "   bind prefix+mod+mod+mod-key command {app,app,app} -e      Hotkey is not enabled while the listed applications have focus\n"
        "   bind prefix+mod+mod+mod-key command {app,app,app} -i      Hotkey is only enabled while the listed applications have focus\n"
        "   unbind mod+mod+mod-key                                    Unbinds hotkeys\n"
        "\n"
        "For further help run:\n"
        "   kwmc help config|window|tree|space|screen|read\n"
    ;
}

void ShowHelp(std::string Command)
{
    if(Command == "config")
    {
        std::cout <<
            "Usage: kwmc config <options>\n"
            "\n"
            "Options:\n"
            "   reload                                                 Reload config ($HOME/.kwm/kwmrc)\n"
            "   spaces-key mod+mod+mod                                 Set modifier used by OSX space-hotkeys (System Preferences->Keyboard->Shortcuts)\n"
            "   prefix mod+mod+mod-key                                 Set prefix for Kwms hotkeys\n"
            "   prefix-global on|off                                   Make prefix global (apply to all binds)\n"
            "   prefix-timeout seconds                                 Set prefix timeout in seconds (default: 0.75)\n"
            "   padding top|bottom|left|right value                    Set default padding\n"
            "   gap vertical|horizontal value                          Set default container gaps\n"
            "   focused-border on|off                                  Enables a border around the focused window\n"
            "   focused-border size number                             Set the width of the border\n"
            "   focused-border color aarrggbb                          Set the color of the border\n"
            "   focused-border radius number                           Set the corner-radius of the border\n"
            "   prefix-border on|off                                   Enables a border to display when prefix is active\n"
            "   prefix-border size number                              Set the width of the border\n"
            "   prefix-border color aarrggbb                           Set the color of the border\n"
            "   prefix-border radius number                            Set the corner-radius of the border\n"
            "   marked-border on|off                                   Enables a border around the marked window\n"
            "   marked-border size number                              Set the width of the border\n"
            "   marked-border color aarrggbb                           Set the color of the border\n"
            "   marked-border radius number                            Set the corner-radius of the border\n"
            "   spawn left|right                                       The container position to be occupied by the new window\n"
            "   capture id application                                 Capture application to screen\n"
            "   float-non-resizable on|off                             Automatically float non-resizable windows\n"
            "   float application                                      Always float application\n"
            "   focus toggle|autofocus|autoraise|off                   Set focus-mode\n"
            "   cycle-focus screen|all|off                             Set wrap-around for 'window -f prev|next'\n"
            "   focus mouse-follows on|off                             Set state of mouse-follows-focus\n"
            "   focus standby-on-float on|off                          Disables focus-follows-mouse when a floating window gains focus\n"
            "   tiling on|off                                          Should Kwm perform tiling\n"
            "   space bsp|monocle|float                                Set default tiling mode\n"
            "   screen id bsp|monocle|float                            Override default tiling mode for screen\n"
            "   hotkeys on|off                                         Set state of Kwm's hotkeys\n"
            "   split-ratio value                                      Set split-ratio to use for containers (0 < value < 1, default: 0.5)\n"
            "   add-role role application\n"
            "        Add custom role for which windows Kwm should tile.\n"
            "        To find the role of a window that Kwm doesn't tile, use the OSX Accessibility Inspector utility.\n"
            "            e.g The following allows Kwm to tile iTerm2 windows that do not have a titlebar\n"
            "            config add-role AXDialog iTerm2\n"
        ;
    }
    else if (Command ==  "window")
    {
        std::cout <<
            "Usage: kwmc window <options>\n"
            "\n"
            "Options:\n"
            "   -t fullscreen|parent|float|mark                        Set window-tiling mode\n"
            "   -c refresh                                             Manually resize window to container\n"
            "   -c split                                               Toggle between vertical and horizontal split for an existing container\n"
            "   -c reduce|expand amount                                Change split-ratio of the focused container (0 < amount < 1)\n"
            "   -f north|east|south|west|prev|next|curr|first|last     Change window focus\n"
            "   -f id windowid                                         Change window focus by id\n"
            "   -s north|east|south|west|prev|next|mark                Swap window position\n"
            "   -x north|east|south|west|mark                          Detach window and reinsert at the given position\n"
            "   -m xoff yoff                                           Change position of a floating window\n"
        ;
    }
    else if (Command == "mark")
    {
        std::cout <<
            "Usage: kwmc mark <options>\n"
            "\n"
            "Options:\n"
            "   -w focused|north|east|south|west wrap|nowrap           Mark a window relative to the focused window\n"
        ;
    }
    else if (Command == "tree")
    {
        std::cout <<
            "Usage: kwmc tree <options>\n"
            "\n"
            "Options:\n"
            "   -r 90|180|270                                          Rotate window-tree\n"
            "   -c refresh                                             Resize all windows to container\n"
            "   -c pseudo create|remove                                Create or remove pseudo-container, filled by next window creation\n"
            "   save name                                              Save current bsp-tree to file ($HOME/.kwm/name)\n"
            "   restore name                                           Load current bsp-tree from file ($HOME/.kwm/layouts/name)\n"
        ;
    }
    else if (Command ==  "space")
    {
        std::cout <<
            "Usage: kwmc space <options>\n"
            "\n"
            "Options:\n"
            "   -t toggle|bsp|monocle|float                             Set tiling mode of current space (every space uses bsp tiling by default)\n"
            "   -p increase|decrease left|right|top|bottom              Change space padding\n"
            "   -g increase|decrease vertical|horizontal                Change space container gaps\n"
            "   -s id num                                               Change space of current display (System Preferences->Keyboard->Shortcuts for spaces must be enabled!)\n"
        ;
    }
    else if (Command ==  "screen")
    {
        std::cout <<
            "Usage: kwmc screen <options>\n"
            "\n"
            "Options:\n"
            "   -s optimal|vertical|horizontal                         Set split-mode\n"
            "   -f prev|next|id                                        Change monitor focus\n"
            "   -m prev|next|id                                        Move window between monitors (id of primary monitor is 0 and last monitor n-1)\n"
        ;
    }
    else if (Command == "read")
    {
        std::cout <<
            "Usage: kwmc read <options>\n"
            "\n"
            "Options:\n"
            "   focused                                                Get owner and title of focused window\n"
            "   tag                                                    Get tag for current space\n"
            "   current                                                Get id of focused window (-1 == not marked)\n"
            "   marked                                                 Get id of marked window (-1 == not marked)\n"
            "   parent windowid                                        Check if the focused window and a window have the same parent\n"
            "   dir south|north|east|west wrap|nowrap                  Get id of the window in direction of focused window\n"
            "   child windowid                                         Get child position of window from parent (left or right child)\n"
            "   prefix                                                 Get state of the prefix-key\n"
            "   space                                                  Get tiling mode for current space\n"
            "   cycle-focus                                            Get active cycle-focus mode\n"
            "   focus                                                  Get state of focus-follows-mouse\n"
            "   mouse-follows                                          Get state of mouse-follows-focus\n"
            "   spawn                                                  Get state of 'kwmc config spawn'\n"
            "   split-mode global                                      Get the current mode used for binary splits\n"
            "   split-mode windowid                                    Get the split-mode used for the given window\n"
            "   split-ratio                                            Get the current ratio used for binary splits\n"
            "   border focused|marked|prefix                           Get the state of border->enable\n"
            "   windows                                                Get list of visible windows on active space\n"
        ;
    }
    else
    {
      ShowUsage();
    }
}
