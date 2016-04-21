*Kwmc* is a program used to write to *Kwm*'s socket.
The following are instructions for enabling the man file
```
gzip kwmc.1
cp kwmc.1.gz /usr/local/share/man/man1
man kwmc
```

### Glossary

        A command with <opt> or <arg> means that an argument of that type is required
        A command with [opt] means that an argument is optional

### Configure Kwm
        Reload config ($HOME/.kwm/kwmrc)
            kwmc config reload

        Set modifier used by OSX space-hotkeys (Used by `kwmc space -f ..`
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

        Set window border radius
            kwmc config <opt>-border radius <arg>
            <opt>: focused | marked | prefix
            <arg>: number

        The container position to be occupied by new windows
            kwmc config spawn <opt>
            <opt>: left | right

        Automatically float non-resizable windows
            kwmc config float-non-resizable <opt>
            <opt>: on | off

        Automatically reapply container if window changes size
            kwmc config lock-to-container <opt>
            <opt>: on | off

        Set status of focus-follows-mouse
            kwmc config focus-follows-mouse <opt>
            <opt>: toggle | autofocus | autoraise | off

        Disable focus-follows-mouse when a floating window gains focus
            kwmc config standby-on-float <opt>
            <opt>: on | off

        Allow focus commands to wrap
            kwmc config cycle-focus <opt>
            <opt>: screen | off

        Set state of mouse-follows-focus
            kwmc config mouse-follows-focus <opt>
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

        Override default tiling mode for space
            kwmc config space display_id space_id mode <opt>
            <opt>: bsp | monocle | float

        Override default padding for space
            kwmc config space display_id space_id padding <opt>
            <opt>: top bottom left right

        Override default gaps for space
            kwmc config space display_id space_id gap <opt>
            <opt>: vertical horizontal

        Override default tiling mode for display
            kwmc config display display_id mode <opt>
            <opt>: bsp | monocle | float

        Override default padding for display
            kwmc config display display_id padding <opt>
            <opt>: top bottom left right

        Override default gap for display
            kwmc config display display_id gap <opt>
            <opt>: vertical horizontal

        Enable hotkeys registered using `bind`
            kwmc config hotkeys <opt>
            <opt>: on | off

        Set split-ratio for containers
            kwmc config split-ratio <opt>
            <opt>: 0 < floating point number < 1

        Create a hotkey consumed by Kwm
            kwmc bind prefix+mod+mod+mod-key command [opt]
            [opt]: {app,app,app} -e | {app,app,app} -i
                    -e: not enabled for listed applications
                    -i: only enabled for listed applications

        Create a hotkey not consumed by Kwm
            kwmc bind-passthrough prefix+mod+mod+mod-key command [opt]
            [opt]: {app,app,app} -e | {app,app,app} -i

        Unbind a hotkey
            kwmc unbind <opt>
            <opt>: mod+mod+mod-key

        Add custom role for which windows Kwm should tile
            kwmc config add-role AXRole <opt>
            <opt>: name of application

            The following allows Kwm to tile borderless iTerm2
                kwmc config add-role AXDialog iTerm2

        Create rules that applies to specific applications or windows
        See (https://github.com/koekeishiya/kwm/issues/268) for details
            kwmc rule owner="" name="" properties={float=""; display=""} except=""

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
            kwmc window -f <opt>
            <opt>: north | east | south | west | prev | next | curr | window_id

        Change focus between monocle-subtree windows
            kwmc window -fm <opt>
            <opt>: prev | next

        Swap window position
            kwmc window -s <opt>
            <opt>: north | east | south | west | prev | next | mark

        Adjust container zoom
            kwmc window -z <opt>
            <opt>: fullscreen | parent

        Toggle window floating
            kwmc window -t <opt>
            <opt>: focused

        Resize window to container size
            kwmc window -r <opt>
            <opt>: focused

        Modify container split-mode of window
            kwmc window -c split-mode <opt>
            <opt>: toggle

        Modify container type of window
            kwmc window -c type <opt>
            <opt>: monocle | bsp | toggle

        Modify container split-ratio of window
            kwmc window -c reduce <opt>
            kwmc window -c expand <opt>
            <opt>: 0 < floating point number < 1

        Modify container split-ratio of window in direction
            kwmc window -c reduce <opt> <arg>
            kwmc window -c expand <opt> <arg>
            <opt>: 0 < floating point number < 1
            <arg>: north | east | south | west

        Move window on the current space
            kwmc window -m <opt>
            <opt>: north | east | south | west | mark | xoff yoff

        Move window to a different space
            kwmc window -m space <opt>
            <opt>: workspace_id | previous | left | right

        Move window to an external display
            kwmc window -m display <opt>
            <opt>: display_id | prev | next

        Mark the focused window
            kwmc window -mk focused

        Mark window in direction of focused window
            kwmc window -mk <opt> <arg>
            <opt>: north | east | south | west
            <arg>: wrap | nowrap

        Change focus between spaces
            kwmc space -f <opt>
            <opt>: workspace_id | previous | left | right

        Change focus between spaces, skipping transitions animation
        (mission control does not update, requires `killall Dock`)
            kwmc space -fExperimental <opt>
            <opt>: workspace_id | previous | left | right

        Set tiling mode of space
            kwmc space -t <opt>
            <opt>: bsp | monocle | float

        Resize all windows to their container size
            kwmc space -r <opt>
            <opt>: focused

        Adjust padding
            kwmc space -p <opt> <arg>
            <opt>: increase | decrease
            <arg>: all | left | right | top | bottom

        Adjust gaps
            kwmc space -g <opt> <arg>
            <opt>: increase | decrease
            <arg>: all | vertical | horizontal

        Change focus between displays
            kwmc display -f <opt>
            <opt>: prev | next | display_id

        Set active split-mode of display
            kwmc display -c <opt>
            <opt>: optimal | vertical | horizontal

        Manage pseudo containers
            kwmc tree -pseudo <opt>
            <opt>: create | destroy

        Rotate window-tree of current space
            kwmc tree rotate <opt>
            <opt>: 90 | 180 | 270

        Save bsp-layout of window-tree of current space
            kwmc tree save <opt>
            <opt>: filename

        Restore bsp-layout of window-tree of current space
            kwmc tree restore <opt>
            <opt>: filename

### Query current state

        Get owner and title of focused window
            kwmc query focused

        Get tag for current space
            kwmc query tag

        Get id of focused window (-1 == none)
            kwmc query current

        Get id of marked window (-1 == none)
            kwmc query marked

        Get child position of window from parent (left or right child)
            kwmc query child windowid

        Get id of the window in direction of focused window
            kwmc query dir south|north|east|west wrap|nowrap

        Check if the focused window and a window have the same parent node
            kwmc query parent windowid

        Get state of 'kwmc config spawn'
            kwmc query spawn

        Get state of the prefix-key
            kwmc query prefix

        Get tilling mode to use for new spaces
            kwmc query space

        Get active cycle-focus mode
            kwmc query cycle-focus

        Get state of focus-follows-mouse
            kwmc query focus

        Get state of mouse-follows-focus
            kwmc query mouse-follows

        Get the split-mode for the given window
            kwmc query split-mode windowid

        Get the current mode used for binary splits
            kwmc query split-mode global

        Get the current ratio used for binary splits
            kwmc query split-ratio

        Get the state of border->enable
            kwmc query border focused|marked|prefix

        Get list of visible windows on active space
            kwmc query windows

        Get id of previous active space for the focused display
            kwmc query prev-space
