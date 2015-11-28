## Description

Kwm started as a simple project to get true focus-follows-mouse support on OSX through event tapping.

It is a tiling window manager that represents windows as the leaves of a binary tree.

Tested on Osx El Capitan (10.11.1).

![alt tag](https://cloud.githubusercontent.com/assets/6175959/11390251/8c8b6952-9348-11e5-9e4d-e77152f7536f.png)

## Features:
- Enables focus-follows-mouse
- Binary space partitioning window management
- System wide hotkeys (supports instantaneous live code editing)
- Remapping of keys (supports instantaneous live code editing)

## Requirements:
- Access to osx accessibility

## Hotkeys:
    - ctrl+alt+cmd:
        - t: toggle focus mode (focus-follows-mouse -> autoraise -> disabled)
        - r: manually resize window to its containersize
        - q: restart kwm

        - m: mark the container to use for next window split

        - o: use width/height ratio to determine optimal split (default)
        - 7: use vertical split-mode
        - -: use horizontal split-mode

        - h: move vertical-splitter left (increase width of right-pane containers)
        - l: move vertical-splitter right (increase width of left-pane containers)
        - j: move horizontal-splitter down (increase height of upper-pane containers)
        - k: move horizontal-splitter up (increase height of lower-pane containers)

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

        - larrow: increase screen padding-left 
        - rarrow: increase screen padding-right 
        - uarrow: increase screen padding-top 
        - darrow: increase screen padding-bottom 

    - alt+cmd
        - h: focus previous window
        - l: focus next window

        - p: swap with the previous window
        - n: swap with the next window
        - r: reflect window-tree vertically

        - larrow: decrease screen padding-left 
        - rarrow: decrease screen padding-right 
        - uarrow: decrease screen padding-top 
        - darrow: decrease screen padding-bottom 

## Build:

To compile Kwm, simply run the included build.sh script.

By default, debug prints are enabled, and these can be turned off by opening the build.sh script and getting rid of the -DDEBUG_BUILD flag.

Because there is no app bundle, Kwm has to be started from a terminal.

cd into the containing directory and type ./kwm

## Usage:

Whenever Kwm is started, it will automatically tile the currently opened windows, using binary space partitioning.

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

By default, Kwm will always split the focused container, however you can override this
by focusing a window to split, and hit the hotkey cmd+alt+ctrl+m.

This marks the container of the focused window, and will now split this container
when opening a new window.

## Launch from Path:

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
