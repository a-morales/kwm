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
        "   tree                                                      Manipulate current window-tree\n"
        "   read                                                      Retrieve current Kwm settings\n"
        "   write sentence                                            Automatically emit keystrokes to the focused window\n"
        "   press mod+mod+mod-key                                     Send a key press with the specified modifiers\n"
        "   bind prefix+mod+mod+mod-key command                       Create a global hotkey (use `sys` prefix for non kwmc command)\n"
        "   bind prefix+mod+mod+mod-key command {app,app,app} -e      Hotkey is not enabled while the listed applications have focus\n"
        "   bind prefix+mod+mod+mod-key command {app,app,app} -i      Hotkey is only enabled while the listed applications have focus\n"
        "   unbind mod+mod+mod-key                                    Unbinds hotkeys\n"
        "\n"
        "For further help run:\n"
        "   kwmc help config|tree|read\n"
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
            "   prefix-key mod+mod+mod-key                             Set prefix for Kwms hotkeys\n"
            "   prefix-global on|off                                   Make prefix global (apply to all binds)\n"
            "   prefix-timeout seconds                                 Set prefix timeout in seconds (default: 0.75)\n"
            "   padding top bottom left right                          Set default padding\n"
            "   gap vertical horizontal                                Set default container gaps\n"
            "   optimal-ratio num                                      Override the value used by optimal split-mode (default: golden ratio -> 1.618)\n"
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
            "   lock-to-container on|off                               Automatically reapply container if window changes size\n"
            "   float-non-resizable on|off                             Automatically float non-resizable windows\n"
            "   float application                                      Always float application\n"
            "   focus toggle|autofocus|autoraise|off                   Set focus-mode\n"
            "   cycle-focus screen|off                                 Set wrap-around for 'window -f prev|next'\n"
            "   focus mouse-follows on|off                             Set state of mouse-follows-focus\n"
            "   focus standby-on-float on|off                          Disables focus-follows-mouse when a floating window gains focus\n"
            "   tiling bsp|monocle|float|off                           Set default tiling mode for Kwm\n"
            "   space screenid spaceid mode bsp|monocle|float          Override default tiling mode for space\n"
            "   space screenid spaceid padding top bottom left right   Override default padding for space\n"
            "   space screenid spaceid gap vertical horizontal         Override default padding for space\n"
            "   screen id mode bsp|monocle|float                       Override default tiling mode for screen\n"
            "   screen id padding top bottom left right                Override default padding for screen\n"
            "   screen id gap vertical horizontal                      Override default padding for screen\n"
            "   hotkeys on|off                                         Set state of Kwm's hotkeys\n"
            "   split-ratio value                                      Set split-ratio to use for containers (0 < value < 1, default: 0.5)\n"
            "   add-role role application\n"
            "        Add custom role for which windows Kwm should tile.\n"
            "        To find the role of a window that Kwm doesn't tile, use the OSX Accessibility Inspector utility.\n"
            "            e.g The following allows Kwm to tile iTerm2 windows that do not have a titlebar\n"
            "            config add-role AXDialog iTerm2\n"
        ;
    }
    else if (Command == "tree")
    {
        std::cout <<
            "Usage: kwmc tree <options>\n"
            "\n"
            "Options:\n"
            "   rotate 90|180|270                                      Rotate window-tree\n"
            "   save name                                              Save current bsp-tree to file ($HOME/.kwm/name)\n"
            "   restore name                                           Load current bsp-tree from file ($HOME/.kwm/layouts/name)\n"
        ;
    }
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
