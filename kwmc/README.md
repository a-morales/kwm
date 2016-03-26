*Kwmc* is a program used to write to *Kwm*'s socket

### Configure Kwm
        Reload config ($HOME/.kwm/kwmrc)
            kwmc config reload

        Set modifier used by OSX space-hotkeys
            kwmc config spaces-key <opt>
            <opt>: mod+mod+mod

        Set a prefix for Kwms hotkeys
            kwmc config prefix-key <opt>
            <opt>: mod+mod+mod-key

        Make prefix global
            kwmc config prefix-global <opt>
            <opt>: on | off

        Set prefix timeout in seconds
            kwmc config prefix-timeout <opt>
            <opt>: floating point number

        Override the optimal split-mode (golden ratio -> 1.618)
            kwmc config optimal-ratio <opt>
            <opt>: floating point number

        Enable window border
            kwmc config <opt>-border <arg>
            <opt>: focused | marked | prefix
            <arg>: on | off

        Set window border thickness
            kwmc config <opt>-border size <arg>
            <opt>: focused | marked | prefix
            <arg>: number

        Set window border color
            kwmc config <opt>-border color <arg>
            <opt>: focused | marked | prefix
            <arg>: aarrggbb

        The container position to be occupied by new windows
            kwmc config spawn <opt>
            <opt>: left | right

        Automatically float non-resizable windows
            kwmc config float-non-resizable <opt>
            <opt>: on | off

        Automatically reapply container if window changes size
            kwmc config lock-to-container <opt>
            <opt>: on | off

        Always float application
            kwmc config float <opt>
            <opt>: name of application

        Capture application to display
            kwmc config capture <opt>
            <opt>: display_id <arg>
            <arg>: name of application

        Set focus-mode
            kwmc config focus <opt>
            <opt>: toggle | autofocus | autoraise | off

        Disable focus-follows-mouse when a floating window gains focus
            kwmc config focus standby-on-float <opt>
            <opt>: on | off

        Allow focus commands to wrap
            kwmc config cycle-focus <opt>
            <opt>: screen | off

        Set state of mouse-follows-focus
            kwmc config focus mouse-follows <opt>
            <opt>: on | off

        Set default tiling mode for Kwm
            kwmc config tiling <opt>
            <opt>: bsp | monocle | float | off

        Set default padding
            kwmc config padding <opt>
            <opt>: top bottom left right

        Set default container gaps
            kwmc config gap <opt>
            <opt>: vertical horizontal

        Override default settings for space
            kwmc config space <opt>
            <opt>: display_id workspace_id <arg>
            <arg>: mode <arg2> | padding <arg3> | gap <arg4>
            <arg2>: bsp | monocle | float
            <arg3>: top bottom left right
            <arg4>: vertical horizontal

        Override default settings for screen
            kwmc config screen <opt>
            <opt>: display_id <arg>
            <arg>: mode <arg2> | padding <arg3> | gap <arg4>
            <arg2>: bsp | monocle | float
            <arg3>: top bottom left right
            <arg4>: vertical horizontal

        Enable hotkeys registered using `bind`
            kwmc config hotkeys <opt>
            <opt>: on | off

        Set split-ratio for containers
            kwmc config split-ratio <opt>
            <opt>: 0 < floating point number < 1

        Create a hotkey consumed by Kwm
            kwmc bind prefix+mod+mod+mod-key <opt>
            <opt>: command | command <arg>
            <arg>: {app,app,app} -e | {app,app,app} -i
                -e: not enabled for listed applications
                -i: only enabled for listed applications

        Create a hotkey not consumed by Kwm
            kwmc bind-passthrough prefix+mod+mod+mod-key <opt>
            <opt>: command | command <arg>
            <arg>: {app,app,app} -e | {app,app,app} -i

        Unbind a hotkey
            kwmc unbind <opt>
            <opt>: mod+mod+mod-key

        Add custom role for which windows Kwm should tile
            kwmc config add-role <opt>
            <opt>: role <arg>
            <arg>: name of application

            The following allows Kwm to tile borderless iTerm2
                kwmc config add-role AXDialog iTerm2

### Interact with Kwm

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
            <opt>: north | east | south | west | prev | next | curr | window_id

        Change focus between monocle-subtree windows
            kwmc focus -sub-window <opt>
            <opt>: prev | next

        Change focus between spaces
            kwmc focus -space <opt>
            <opt>: workspace_id

        Change focus between displays
            kwmc focus -display <opt>
            <opt>: prev | next | display_id

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
            <arg2>: 0 < floating point number < 1

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
            <arg1>: display_id | prev | next
            <arg2>: workspace_id | left | right

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

### Query current state

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
