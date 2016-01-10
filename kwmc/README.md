*Kwmc* is a program used to write to *Kwm*'s socket

## Kwmc Info:
    Configure Kwm
        Reload config ($HOME/.kwmrc)
            kwmc config reload

        Let launchd manage Kwm (automatically start on login)
            kwmc config launchd enable|disable

        Set a global prefix for all of Kwms hotkeys (optional)
            kwmc config prefix mod+mod+mod-key

        Set global prefix timeout in seconds (default: 0.75)
            kwmc config prefix-timeout seconds

        Set default padding
            kwmc config padding top|bottom|left|right value

        Set default container gaps
            kwmc config gap vertical|horizontal value

        Always float application
            kwmc config float application

        Capture application to screen (Always open on the specified screen)
            kwmc config capture id application

        Set focus-mode
            kwmc config focus toggle|autofocus|autoraise|disabled

        Set focus-wrap-around for 'window -f prev|next'
            kwmc config cycle-focus screen|all|disabled

        Set state of mouse-follows-focus
            kwmc config focus mouse-follows enable|disable

        Should Kwm perform tiling
            kwmc config tiling enable|disable

        Set default tiling mode
            kwmc config space bsp|monocle|float

        Set state of Kwm's hotkeys
            kwmc config hotkeys enable|disable

        Allow drag&drop to make window floating
            kwmc config dragndrop enable|disable

        Prevent focus-follows-mouse if context-menus|menubar is visible
            kwmc config menu-fix enable|disable

        Set split-ratio to use for containers (0 < value < 1, default: 0.5)
            kwmc config split-ratio value

        Create hotkeys on the fly (use `sys` prefix for non kwmc command)
            kwmc unbind mod+mod+mod-key
            kwmc bind mod+mod+mod-key command
                e.g: kwmc bind cmd+alt-l window -f next
                e.g: kwmc unbind cmd+alt-l

        Add custom role for which windows Kwm should tile.
        To find the role of a window that Kwm doesn't tile, 
        Use the OSX Accessibility Inspector utility.
            kwmc config add-role role application

                e.g The following allows Kwm to tile iTerm2 windows that do not have a titlebar
                kwmc config add-role AXDialog iTerm2


    Quit Kwm
        kwmc quit

    Get owner and title of focused window
        kwmc focused

    Automatically emit keystrokes to the focused window
        kwmc write sentence

    Set window-tiling mode
        kwmc window -t fullscreen|parent|float|mark

    Resize window to container
        kwmc window -c refresh

    Toggle between vertical and horizontal split for an existing container
        kwmc window -c split

    Change split-ratio of the focused container (0 < amount < 1)
        kwmc window -c reduce|expand amount

    Change window focus
        kwmc window -f prev|next|curr

    Swap window position
        kwmc window -s prev|next|mark

    Rotate window-tree
        kwmc tree -r 90|180|270

    Resize all windows to container
        kwmc tree -c refresh

    Set tiling mode of current space (every space uses bsp tiling by default)
        kwmc space -t toggle|bsp|monocle|float

    Change space padding
        kwmc space -p increase|decrease left|right|top|bottom

    Change space container gaps
        kwmc space -g increase|decrease vertical|horizontal

    Set split-mode
        kwmc screen -s optimal|vertical|horizontal

    Move window between monitors (id of primary monitor is 0 and last monitor n-1)
        kwmc screen -m prev|next|id

    Give focus to monitor
        kwmc screen -f prev|next|id
