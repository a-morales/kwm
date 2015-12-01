## Description

*Kwm* started as a simple project to get true focus-follows-mouse support on OSX through event tapping.

It is a tiling window manager that represents windows as the leaves of a binary tree.

*Kwm* uses the event taps API (Quartz Event Services) to observe, filter and alter user input events
prior to their delivery to a foreground application.

This allows for functionality such as focus-follows-mouse, remapping keys, and most importantly 
global hotkeys, mainly to be used for interaction with the parts of *Kwm* that are exposed to hotkeys.cpp
through the export_table struct, and so hotkeys.cpp can and should be customized by the user.

hotkeys.cpp can be edited and recompiled separately, thus any changes made does not require *Kwm* to be restarted.


*Multiple monitor support:*
Currently *Kwm* only looks for active monitors upon startup, and so if a monitor is connected or disconnected
while *Kwm* is running, it has to be restarted.
    
There is no built-in way to send a window to a different monitor as of now, however a workaround for this is
by first toggling thewindow focused and then dragging it to whatever monitor.

Spaces will eventually get their own padding and gap settings.


*Kwm* requires access to osx accessibility.  Creating a certificate and codesigning the binary works as well.

Tested on Osx El Capitan (10.11.1).

![alt tag](https://cloud.githubusercontent.com/assets/6175959/11390251/8c8b6952-9348-11e5-9e4d-e77152f7536f.png)

## Build:

To compile *Kwm*, simply run the included build.sh script.

By default, debug prints are enabled, and these can be turned off by opening the build.sh script and
getting rid of the -DDEBUG_BUILD flag.

Because there is no app bundle, *Kwm* has to be started from a terminal.

## Usage:

When *Kwm* is started, it will automatically tile the windows of the current space, using binary space partitioning.

This will also happen once for any other space the user might switch to.

When *Kwm* detects a new window, it inserts it into a window tree at the specified point using the split-mode
specified.

When a window is closed, it will be removed from the window tree and the tree will be rebalanced.

By default, the insertion point is the focused window, but the user can mark a temporary insertion point to be
used instead for the next insertion.

There are 3 types of split-modes available, these are optimal (width/height ratio), vertical and horizontal.

The default split-mode is set to optimal (width/height ratio).

Example:

```
            a                       a                       a
           / \         -->         / \         -->         / \    
          1   2                   1   b                   1   b
                                     / \                     / \
                                    2   3                   c   3
                                                           / \
                                                          2   4

---------------------     ---------------------     --------------------- 
|         |         |     |         |         |     |         |    |    |
|         |         |     |         |    2    |     |         | 2  |  4 |
|         |         |     |         |    *    |     |         |    |    |
|    1    |    2    |     |    1    |---------|     |    1    |---------|
|         |    *    |     |         |         |     |         |         |
|         |         |     |         |    3    |     |         |    3    |
|         |         |     |         |         |     |         |         |
---------------------     ---------------------     ---------------------

```

*Kwm* mainly consists of three panes so to speak. The main pane being the entire screen.

After at least 1 window insertion has occurred, the screen will consist of either a left and right pane,
or an upper and lower pane.

The width of the left/right pane can be changed, as well as the height of the upper/lower pane

Currently these cannot occur simultaneously, and the type of panes created is decided by the main pane split-mode.

If the main pane is split using vertical mode, a left and right pane is created.

If the main pane is split using horizontal mode, an upper and lower pane is created.

If *Kwm* is added to the path, a launcher script is necessary
because it tries to load hotkeys.so from the executables working-directory.

Example launcher script to be placed in the path instead of the *Kwm* binary.

    #!/bin/bash
    pushd /path/to/kwm
    ./kwm
    popd

The reason for this is that hotkeys.cpp can edited and rebuild separately,
and *Kwm* will reload this library without having to be restarted and so
hotkeys can be edited live.

## Default Hotkeys:
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

        - x: increase horizontal gap
        - y: increase vertical gap

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

        - x: decrease horizontal gap
        - y: decrease vertical gap

        - larrow: decrease screen padding-left 
        - rarrow: decrease screen padding-right 
        - uarrow: decrease screen padding-top 
        - darrow: decrease screen padding-bottom 
