*Kwmc* is a program used to write to *Kwm*'s socket

## Kwmc Info:
    Configure Kwm
        Reload config ($HOME/.kwm/kwmrc)
            kwmc config reload

        Let launchd manage Kwm (automatically start on login)
            kwmc config launchd enable|disable

        Set modifier used by OSX space-hotkeys (System Preferences->Keyboard->Shortcuts)
            kwmc config spaces-key mod+mod+mod

        Set a prefix for Kwms hotkeys (optional)
            kwmc config prefix mod+mod+mod-key

        Make prefix global (apply for all binds)
            kwmc config prefix-global enable|disable

        Set prefix timeout in seconds (default: 0.75)
            kwmc config prefix-timeout seconds

        Set default padding
            kwmc config padding top|bottom|left|right value

        Set default container gaps
            kwmc config gap vertical|horizontal value

        Customize focused-window border
            kwmc config focused-border enable|disable
            kwmc config focused-border size number
            kwmc config focused-border color aarrggbb (hex value)
            kwmc config focused-border radius number

        Customize marked-window border
            kwmc config marked-border enable|disable
            kwmc config marked-border size number
            kwmc config marked-border color aarrggbb (hex value)
            kwmc config marked-border radius number

        Customize border when prefix-key is active
            kwmc config prefix-border enable|disable
            kwmc config prefix-border size number
            kwmc config prefix-border color aarrggbb (hex value)
            kwmc config prefix-border radius number

        The container position to be occupied by the new window
            kwmc config spawn left|right

        Automatically float non-resizable windows
            kwmc config float-non-resizable enable|disable

        Always float application
            kwmc config float application

        Capture application to screen (Always open on the specified screen)
            kwmc config capture id application

        Set focus-mode
            kwmc config focus toggle|autofocus|autoraise|disabled

        Disable focus-follows-mouse when a floating window gains focus
            kwmc config focus standby-on-float enable|disable

        Set focus-wrap-around for 'window -f prev|next'
            kwmc config cycle-focus screen|all|disabled

        Set state of mouse-follows-focus
            kwmc config focus mouse-follows enable|disable

        Should Kwm perform tiling
            kwmc config tiling enable|disable

        Set default tiling mode
            kwmc config space bsp|monocle|float

        Override default tiling mode for screen
            kwmc config screen id bsp|monocle|float

        Set state of Kwm's hotkeys
            kwmc config hotkeys enable|disable

        Allow drag&drop to make window floating
            kwmc config dragndrop enable|disable

        Set split-ratio to use for containers (0 < value < 1, default: 0.5)
            kwmc config split-ratio value

        Create a global hotkey (use `sys` prefix for non kwmc command)
            kwmc bind prefix+mod+mod+mod-key command

        Hotkey is NOT enabled while the listed applications have focus
            kwmc bind prefix+mod+mod+mod-key command {app,app,app} -e

        Hotkey is ONLY enabled while the listed applications have focus
            kwmc bind prefix+mod+mod+mod-key command {app,app,app} -i

        Unbind a hotkey that has already been set
            kwmc unbind mod+mod+mod-key

        Add custom role for which windows Kwm should tile.
        To find the role of a window that Kwm doesn't tile,
        Use the OSX Accessibility Inspector utility.
            kwmc config add-role role application

                e.g The following allows Kwm to tile iTerm2 windows that do not have a titlebar
                kwmc config add-role AXDialog iTerm2


    Commands to interact with Kwm
        Quit Kwm
            kwmc quit

        Automatically emit keystrokes to the focused window
            kwmc write sentence

        Send a key press with the specified modifiers
            kwmc press mod+mod+mod-key

        Set window-tiling mode
            kwmc window -t fullscreen|parent|float

        Resize window to container
            kwmc window -c refresh

        Toggle between vertical and horizontal split for an existing container
            kwmc window -c split

        Change split-ratio of the focused container (0 < amount < 1)
            kwmc window -c reduce|expand amount

        Change window focus
            kwmc window -f north|east|south|west|prev|next|curr|first|last

        Change window focus by id
            kwmc window -f id windowid

        Swap window position
            kwmc window -s north|east|south|west|prev|next|mark

        Detach window and reinsert at the given position
            kwmc window -x north|east|south|west|mark

        Change position of a floating window
            kwmc window -m xoff yoff

        Mark a window relative to the focused window
            kwmc mark -w focused|north|east|south|west wrap|nowrap

        Rotate window-tree
            kwmc tree -r 90|180|270

        Resize all windows to container
            kwmc tree -c refresh

        Save current bsp-tree to file ($HOME/.kwm/name)
            kwmc tree save name

        Restore current bsp-tree from file ($HOME/.kwm/name)
            kwmc tree restore name

        Change space of current display (System Preferences->Keyboard->Shortcuts for spaces must be enabled!)
            kwmc space -s id num

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


    Get state of Kwm
        Get owner and title of focused window
            kwmc read focused

        Get tag for current space
            kwmc read tag

        Get id of focused window (-1 == none)
            kwmc read current

        Get id of marked window (-1 == none)
            kwmc read marked

        Get child position of window from parent (left or right child)
            kwmc read child windowid

        Get id of the window in direction of focused window
            kwmc read dir south|north|east|west wrap|nowrap

        Check if the focused window and a window have the same parent node
            kwmc read parent windowid

        Get state of 'kwmc config spawn'
            kwmc read spawn

        Get state of the prefix-key
            kwmc read prefix

        Get tilling mode for current space
            kwmc read space

        Get active cycle-focus mode
            kwmc read cycle-focus

        Get state of focus-follows-mouse
            kwmc read focus

        Get state of mouse-follows-focus
            kwmc read mouse-follows

        Get the split-mode for the given window
            kwmc read split-mode windowid

        Get the current mode used for binary splits
            kwmc read split-mode global

        Get the current ratio used for binary splits
            kwmc read split-ratio

        Get the state of border->enable
            kwmc read border focused|marked|prefix

        Get list of visible windows on active space
            kwmc read windows
