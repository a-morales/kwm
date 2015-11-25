## Kwm

Started as a simple project to get true focus-follows-mouse support on OSX through event tapping.
Scroll down for instructions on how to use.

![alt tag](https://cloud.githubusercontent.com/assets/6175959/11104379/a4271a1e-88c8-11e5-805e-5d3e0e83d267.png)

### Features:
- Enables focus-follows-mouse
- Binary space partitioning window management
- System wide hotkeys (supports instantaneous live code editing)
- Remapping of keys (supports instantaneous live code editing)

### Requirements:
- Access to osx accessibility

### Hotkeys:
    - ctrl+alt+cmd:
        - t: toggle focus mode (focus-follows-mouse -> autoraise -> disabled)
        - r: manually resize window to its containersize
        - q: restart kwm

        - m: mark the container to use for next window split

        - o: use width/height ratio to determine optimal split (default)
        - 7: use vertical split-mode
        - -: use horizontal split-mode

        - h: move vertical-splitter left
             (increase width of right-pane containers)

        - l: move vertical-splitter right
             (increase width of left-pane containers)

        - j: move horizontal-splitter down
             (increase height of upper-pane containers)

        - k: move horizontal-splitter up
             (increase height of lower-pane containers)

        - f: toggle window fullscreen
        - p: toggle window parent container
        - w: toggle window floating
        - enter: opens a new iTerm window

        YTD player controls:
        - <: toggle playback mode (fav / default)
        - x: toggle play/pause
        - v: stop player

        - s: star / favorite playing song

        - a: increase volume
        - d: decrease volume
        
        - z: play previous video
        - c: play next video

        - larrow: seek backward
        - rarrow: seek forward

    - ctrl+alt:
        - p: send window to previous screen
        - n: send window to next screen

    - alt+cmd
        - h: focus previous window
        - l: focus next window

        - p: swap with the previous window
        - n: swap with the next window

### How to use:

To compile Kwm, simply run the included build.sh script.
By default, debug prints are enabled, and these can be turned
off by opening the build.sh script and getting rid of the -DDEBUG_BUILD flag.

Because there is no app bundle, Kwm has to be started from
a terminal. cd into the containing directory and type ./kwm

Whenever Kwm is started, it will automatically tile the currently
opened windows, using binary space partitioning.

There are 3 types of split modes available, optimal (width/height ratio), vertical and horizontal split.
The user can toggle between these modes using the hotkeys:

    optimal cmd+alt+ctrl+o
    vertical cmd+alt+ctrl+7
    horizontal cmd+alt+ctrl+-

Kwm mainly consists of three panes so to speak. The main pane is the entire screen.
(minus whatever screen-padding set in display.cpp)

After at least 1 split has occurred, the screen will consist of left/right or upper/lower panes.
The width of the left/right pane can be changed using the hotkeys cmd+alt+ctrl+h/l
The height of the upper/lower pane can be changed using the hotkeys cmd+alt+ctrl+j/k

Currently these cannot occur simultaneously, and the type of panes created is decided by the main pane split.
If the main pane is split using vertical mode, a left and right pane is created.
If the main pane is split using horizontal mode, an upper and lower pane is created.

By default, Kwm will always split the left-most container, however you can override this
by focusing a window to split, and hit the hotkey cmd+alt+ctrl+m.
This marks the container of the focused window, and will now split this container
when opening a new window.

If kwm is added to the path, a launcher script is necessary
because it tries to load hotkeys.so from the executables working-directory.

Example launcher script to be placed in the path instead of the kwm binary.

    #!/bin/bash
    pushd /path/to/kwm
    ./kwm
    popd

The reason for this is that hotkeys.cpp can edited and rebuild separately,
and Kwm will reload this library without having to be restarted and so
hotkeys can be edited live.

Kwm also requires access to the accessibility API.
There should be a prompt asking to enable it on startup.
Creating a certificate and codesigning the binary works as well.

NOTE:
Kwm reads com.apple.spaces.plist upon startup.
spaces.plist is invalidated for some reason when OSX
enters sleep, and so Kwms windowing management may break
because of this.

To fix this, re-create all spaces and drag applications to the new spaces.
This causes OSX to update the spaces.plist file.
Restart Kwm and it should work again.
