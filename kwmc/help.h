#ifndef KWMC_H
#define KWMC_H

#include <string>

std::string helpText()
{
    return
      "Usage: kwmc <command>\n"
      "\n"
      "Commands:\n"
      "  config                                                        Configuration options for Kwm\n"
      "      config launchd enable|disable                                 Let launchd manage Kwm (automatically start on login)\n"
      "      config padding top|bottom|left|right value                    Set default padding\n"
      "      config gap vertical|horizontal value                          Set default container gaps\n"
      "      config float application                                      Always float application\n"
      "      config focus toggle|autofocus|autoraise|disabled              Set focus-mode\n"
      "      config focus mouse-follows enable|disable                     Set state of mouse-follows-focus\n"
      "      config tiling enable|disable                                  Should Kwm perform tiling\n"
      "      config space bsp|monocle|float                                Set default tiling mode\n"
      "      config hotkeys enable|disable                                 Set state of Kwm's hotkeys\n"
      "      config dragndrop enable|disable                               Allow drag&drop to make window floating\n"
      "      config menu-fix enable|disable                                Prevent focus-follows-mouse if context-menus|menubar is visible\n"
      "      config split-ratio value                                      Set split-ratio to use for containers (0 < value < 1, default: 0.5)\n"
      "      add-role role application\n"
      "          Add custom role for which windows Kwm should tile.\n"
      "          To find the role of a window that Kwm doesn't tile, use the OSX Accessibility Inspector utility.\n"
      "              e.g The following allows Kwm to tile iTerm2 windows that do not have a titlebar\n"
      "              config add-role AXDialog iTerm2\n"
      "  quit                                                          Quit Kwm\n"
      "  help                                                          Display this screen\n"
      "  focused                                                       Get owner and title of focused window\n"
      "  write sentence                                                Automatically emit keystrokes to the focused window\n"
      "  window -t fullscreen|parent|float|mark                        Set window-tiling mode\n"
      "  window -c refresh                                             Manually resize window to container\n"
      "  window -c split                                               Toggle between vertical and horizontal split for an existing container\n"
      "  window -c reduce|expand amount                                Change split-ratio of the focused container (0 < amount < 1)\n"
      "  window -f prev|next|curr                                      Change window focus\n"
      "  window -s prev|next|mark                                      Swap window position\n"
      "  space -t toggle|bsp|monocle|float                             Set tiling mode of current space (every space uses bsp tiling by default)\n"
      "  space -r 90|180|270                                           Rotate window-tree\n"
      "  space -p increase|decrease left|right|top|bottom              Change space padding\n"
      "  space -g increase|decrease vertical|horizontal                Change space container gaps\n"
      "  screen -s optimal|vertical|horizontal                         Set split-mode\n"
      "  screen -m prev|next|id                                        Move window between monitors (id of primary monitor is 0 and last monitor n-1)\n"
      "  bind mod+mod+mod-key command                                  Binds hotkeys on the fly (use `sys` prefix for non kwmc command)\n"
      "  unbind mod+mod+mod-key                                        Unbinds hotkeys\n"
    ;
}

#endif
