*Kwmc* is a program used to write to *Kwm*'s socket

## Kwmc Info:
    Configure Kwm
        Set default padding
            kwmc config padding top|bottom|left|right value

        Set default container gaps
            kwmc config gap vertical|horizontal value

        Always float application
            kwmc config float application

        Set state of Kwm's hotkeys
            kwmc config hotkeys enable|disable

        Allow drag&drop to make window floating
            kwmc config dragndrop enable|disable

        Prevent focus-follows-mouse if context-menus|menubar is visible
            kwmc config menu-fix enable|disable

    Quit Kwm
        kwmc quit 

    Get owner and title of focused window
        kwmc focused 

    Set focus-mode
        kwmc focus -t toggle|autofocus|autoraise|disabled

    Set window-tiling mode
        kwmc window -t fullscreen|parent|float|mark

    Manually resize window to container
        kwmc window -c refresh

    Toggle between vertical and horizontal split for an existing container
        kwmc window -c split

    Change window focus
        kwmc window -f prev|next

    Swap window position
        kwmc window -s prev|next|mark

    Set space-tiling mode (every space is tiled by default)
        kwmc space -t toggle|tile|float

    Rotate window-tree
        kwmc space -r 90|180|270

    Move container splitter
        kwmc space -m left|right|up|down

    Change space padding
        kwmc space -p increase|decrease left|right|top|bottom 

    Change space container gaps
        kwmc space -g increase|decrease vertical|horizontal

    Set split-mode
        kwmc screen -s optimal|vertical|horizontal

    Move window between monitors (id of primary monitor is 0 and last monitor n-1)
        kwmc screen -m prev|next|id
