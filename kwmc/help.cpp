#include "help.h"

#include <iostream>

void ShowUsage()
{
    std::cout <<
        "Usage: Kwmc <command>\n"
        "\n"
        "Command:\n"
        "   quit                                         Quit Kwm\n"
        "   help                                         Display this screen\n"
        "   config                                       Set configuration for Kwm\n"
        "   window                                       Manipulate current window\n"
        "   space                                        Manipulate current space\n"
        "   screen                                       Manipulate current screen\n"
        "   focused                                      Get owner and title of focused window\n"
        "   write sentence                               Automatically emit keystrokes to the focused window\n"
        "   bind mod+mod+mod-key command                 Binds hotkeys on the fly (use `sys` prefix for non kwmc command)\n"
        "   unbind mod+mod+mod-key                       Unbinds hotkeys\n"
        "\n"
        "For further help run:\n"
        "   kwmc help config|window|space|screen\n"
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
            "   reload                                                 Reload config ($HOME/.kwmrc)\n"
            "   launchd enable|disable                                 Let launchd manage Kwm (automatically start on login)\n"
            "   prefix mod+mod+mod-key                                 Set global prefix for all of Kwms hotkeys\n"
            "   prefix-timeout seconds                                 Set global prefix timeout in seconds (default: 0.75)\n"
            "   padding top|bottom|left|right value                    Set default padding\n"
            "   gap vertical|horizontal value                          Set default container gaps\n"
            "   float application                                      Always float application\n"
            "   focus toggle|autofocus|autoraise|disabled              Set focus-mode\n"
            "   focus mouse-follows enable|disable                     Set state of mouse-follows-focus\n"
            "   tiling enable|disable                                  Should Kwm perform tiling\n"
            "   space bsp|monocle|float                                Set default tiling mode\n"
            "   hotkeys enable|disable                                 Set state of Kwm's hotkeys\n"
            "   dragndrop enable|disable                               Allow drag&drop to make window floating\n"
            "   menu-fix enable|disable                                Prevent focus-follows-mouse if context-menus|menubar is visible\n"
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
            "   -f prev|next|curr                                      Change window focus\n"
            "   -s prev|next|mark                                      Swap window position\n"
        ;
    }
    else if (Command ==  "space")
    {
        std::cout <<
            "Usage: kwmc space <options>\n"
            "\n"
            "Options:\n"
            "   -t toggle|bsp|monocle|float                             Set tiling mode of current space (every space uses bsp tiling by default)\n"
            "   -r 90|180|270                                           Rotate window-tree\n"
            "   -p increase|decrease left|right|top|bottom              Change space padding\n"
            "   -g increase|decrease vertical|horizontal                Change space container gaps\n"
        ;
    }
    else if (Command ==  "screen")
    {
        std::cout <<
            "Usage: kwmc screen <options>\n"
            "\n"
            "Options:\n"
            "   -s optimal|vertical|horizontal                         Set split-mode\n"
            "   -m prev|next|id                                        Move window between monitors (id of primary monitor is 0 and last monitor n-1)\n"
        ;
    }
    else
    {
      ShowUsage();
    }
}
