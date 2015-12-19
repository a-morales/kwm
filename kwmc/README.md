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

    Set split-mode
        kwmc screen -s optimal|vertical|horizontal

    Rotate window-tree
        kwmc screen -r 90|180|270

    Move container splitter
        kwmc screen -m left|right|up|down

    Move window between monitors
        kwmc screen -m prev|next

    Change screen padding
        kwmc screen -p increase|decrease left|right|top|bottom 

    Change screen container gaps
        kwmc screen -g increase|decrease vertical|horizontal
