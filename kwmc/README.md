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

## Create Keybindings

        Kwm supports two different ways to create a keybind,
        either through the key-symbol or the key-code.

        bindsym: accepts a symbol / letter
        bindcode: accepts a hexadecimal keycode

            kwmc bindsym mode+mod+mod+mod-key command [opt]
            kwmc bindcode mode+mod+mod+mod-hex command [opt]

            [opt]: {app,app,app} -e | {app,app,app} -i
                    -e: not enabled for listed applications
                    -i: only enabled for listed applications

        Any keybinds created with the above commands, will be consumed by Kwm.
        If you wish to have a keybind that performs some action and still passes
        through to the target application, the below command can be used.

            kwmc bind-passthrough mode+mod+mod+mod-key command [opt]
            [opt]: {app,app,app} -e | {app,app,app} -i

        To remove a keybind, the unbind command can be used.
            kwmc unbind mode+mod+mod+mod-key

        Kwm does support multiple binding-modes. A binding-mode is a set of keybinds
        that will only be active when Kwm enters that state. The mode can be specified
        through the `mode` symbol as seen in the above commands. If no mode is set,
        it will be added to a `default` mode.

        A binding-mode can be configured to activate a specific mode when a timeout is reached.
        For this to work, the mode must enable the prefix flag, as well as specify a timeout
        duration. The restore option will default to the `default` mode.
        The color of the focused-border will be changed to the color associated with the active
        mode, if one has been set.

            kwmc mode my_mode prefix on
            kwmc mode my_mode timeout 0.75
            kwmc mode my_mode restore mode_to_activate
            kwmc mode my_mode color aarrggbb

        To activate a certain binding-mode, the following command must be used.
            kwmc mode activate my_mode

        My keybinds can be found [here](https://gist.githubusercontent.com/koekeishiya/8760be63d6171e150278/raw/2b5a6b37e5106570cf02d36ff4e3eed9b11ced9d/Kwm:%2520binds) as an example.

### Configure Kwm
        Reload config ($HOME/.kwm/kwmrc)
            kwmc config reload

        Set modifier used by OSX space-hotkeys (Used by `kwmc space -f ..`
            kwmc config spaces-key <opt>
            <opt>: mod+mod+mod

        Override the optimal split-mode (golden ratio -> 1.618)
            kwmc config optimal-ratio <opt>
            <opt>: floating point number

        Enable window border
            kwmc config <opt>-border <arg>
            <opt>: focused | marked
            <arg>: on | off

        Set window border thickness
            kwmc config <opt>-border size <arg>
            <opt>: focused | marked
            <arg>: number

        Set window border color
            kwmc config <opt>-border color <arg>
            <opt>: focused | marked
            <arg>: aarrggbb

        Set window border radius
            kwmc config <opt>-border radius <arg>
            <opt>: focused | marked
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

        Enable hotkeys registered using `bindsym` and `bindcode`
            kwmc config hotkeys <opt>
            <opt>: on | off

        Set split-ratio for containers
            kwmc config split-ratio <opt>
            <opt>: 0 < floating point number < 1

        Add custom role for which windows Kwm should tile
            kwmc config add-role AXRole <opt>
            <opt>: name of application

            The following allows Kwm to tile borderless iTerm2
                kwmc config add-role AXDialog iTerm2

        Create rules that applies to specific applications or windows
        See (https://github.com/koekeishiya/kwm/issues/268) for details
            kwmc rule owner="" name="" properties={float=""; display=""; space=""} except=""

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

        Get the state of borders
            kwmc query border <opt>
            <opt>: focused | marked

        Get property of focused window
            kwmc query window focused <opt>
            <opt>: id | name | split | float

        Get id of window in direction of focused window
            kwmc query window focused <opt>
            <opt>: north | east | south | west

        Get property of marked window
            kwmc query window marked <opt>
            <opt>: id | name | split | float

        Check if two windows have the same parent
            kwmc query window parent windiw_id1 window_id2

        Get child position of window (left or right)
            kwmc query window child window_id

        Get tilling mode to be used for new spaces
            kwmc query tiling mode

        Get child position used by new windows
            kwmc query tiling spawn

        Get the mode used for binary splits
            kwmc query tiling split-mode

        Get the ratio used for binary splits
            kwmc query tiling split-ratio

        Get tag of the active space
            kwmc query space tag

        Get id of the active space
            kwmc query space active

        Get id of the previously active space
            kwmc query space previous

        Get active cycle-focus mode
            kwmc query cycle-focus

        Get state of float-non-resizable
            kwmc query float-non-resizable

        Get state of lock-to-container
            kwmc query lock-to-container

        Get state of standby-on-float
            kwmc query standby-on-float

        Get state of focus-follows-mouse
            kwmc query focus-follows-mouse

        Get state of mouse-follows-focus
            kwmc query mouse-follows-focus

        Get list of visible windows on active space
            kwmc query window-list
