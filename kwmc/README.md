*Kwmc* is a program used to write to *Kwm*'s socket

## Kwmc Info:
    Configure Kwm
        Reload config ($HOME/.kwm/kwmrc)
            kwmc config reload

        Set modifier used by OSX space-hotkeys (Sys Preferences->Keyboard->Shortcuts)
            kwmc config spaces-key mod+mod+mod

        Set a prefix for Kwms hotkeys (optional)
            kwmc config prefix-key mod+mod+mod-key

        Make prefix global (apply for all binds)
            kwmc config prefix-global on|off

        Set prefix timeout in seconds (default: 0.75)
            kwmc config prefix-timeout seconds

        Override the value used by optimal split-mode (default: golden ratio -> 1.618)
            kwmc config optimal-ratio num

        Customize focused|marked|prefix border
            kwmc config focused|marked|prefix-border on|off
            kwmc config focused|marked|prefix-border size number
            kwmc config focused|marked|prefix-border color aarrggbb
            kwmc config focused|marked|prefix-border radius number

        The container position to be occupied by the new window
            kwmc config spawn left|right

        Automatically float non-resizable windows
            kwmc config float-non-resizable on|off

        Automatically reapply container if window changes size
            kwmc config lock-to-container on|off

        Always float application
            kwmc config float application

        Capture application to screen (Always open on the specified screen)
            kwmc config capture id application

        Set focus-mode
            kwmc config focus toggle|autofocus|autoraise|off

        Disable focus-follows-mouse when a floating window gains focus
            kwmc config focus standby-on-float on|off

        Set focus-wrap-around for 'window -f prev|next'
            kwmc config cycle-focus screen|off

        Set state of mouse-follows-focus
            kwmc config focus mouse-follows on|off

        Set default tiling mode for Kwm
            kwmc config tiling bsp|monocle|float|off

        Set default padding
            kwmc config padding top bottom left right

        Set default container gaps
            kwmc config gap vertical horizontal

        Override default tiling mode for space
            kwmc config space screenid spaceid mode bsp|monocle|float

        Override default padding for space
            kwmc config space screenid spaceid padding top bottom left right

        Override default gaps for space
            kwmc config space screenid spaceid gap vertical horizontal

        Override default tiling mode for screen
            kwmc config screen id mode bsp|monocle|float

        Override default padding for screen
            kwmc config screen id padding top bottom left right

        Override default gaps for screen
            kwmc config screen id gap vertical horizontal

        Set state of Kwm's hotkeys
            kwmc config hotkeys on|off

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

        Automatically emit keystrokes
            kwmc write <opt>
            <opt>: some text

        Send a key press
            kwmc press <opt>
            <opt>: mod+mod+mod-key

        Change focus between windows
            kwmc focus -window <opt>
            <opt>: north | east | south | west | prev | next | curr | window id

        Change focus between monocle-subtree windows
            kwmc focus -sub-window <opt>
            <opt>: prev | next

        Change focus between spaces
            kwmc focus -space <opt>
            <opt>: workspace number

        Change focus between displays
            kwmc focus -display <opt>
            <opt>: prev | next | id

        Swap window position
            kwmc swap -window <opt>
            <opt>: north | east | south | west | prev | next

        Adjust container zoom
            kwmc zoom -window <opt>
            <opt>: fullscreen | parent

        Toggle window floating
            kwmc float -window <opt>
            <opt>: focused

        Make space floating
            kwmc float -space <opt>
            <opt>: focused

        Resize window to container size
            kwmc refresh -window <opt>
            <opt>: focused

        Resize all windows to their container size
            kwmc refresh -space <opt>
            <opt>: focused

        Modify container of window
            kwmc node -window <opt>
            <opt>: type <arg1> | reduce <arg2> | expand <arg2>
            <arg1>: monocle | bsp | toggle
            <arg2>: 0 < amount < 1

        Manage pseudo nodes
            kwmc node -pseudo <opt>
            <opt>: create | destroy

        Set split-mode for node of window
            kwmc split -window <opt>
            <opt>: toggle

        Set split-mode of display
            kwmc split -display <opt>
            <opt>: optimal | vertical | horizontal

        Move window
            kwmc move -window <opt>
            <opt>: display <arg1> | space <arg2> | north | east | south | west | mark | xoff yoff
            <arg1>: display number | prev | next
            <arg2>: workspace number | left | right

        Mark window
            kwmc mark -window <opt>
            <opt>: focused | north <arg> | east <arg> | south <arg> | west <arg>
            <arg>: wrap | nowrap

        Make space tiled
            kwmc tile -space <opt>
            <opt>: bsp | monocle

        Adjust padding
            kwmc padding -space <opt> <arg>
            <opt>: increase | decrease
            <arg>: all | left | right | top | bottom

        Adjust gaps
            kwmc gap -space <opt> <arg>
            <opt>: increase | decrease
            <arg>: all | vertical | horizontal

        Manage window-tree
            kwmc tree <opt>
            <opt>: rotate <arg1> | save <arg2> | restore <arg2>
            <arg1>: 90 | 180 | 270
            <arg2>: filename

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

        Get tilling mode to use for new spaces
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
