## Description

*Kwm* started as a simple project to get true focus-follows-mouse support on OSX through event tapping.  
It is now a tiling window manager that represents windows as the leaves of a binary tree.

*Kwm* runs a local daemon to read messages and trigger functions that affect focus management and tiling
among other things.

*Kwmc* can be used to write to *Kwm*'s socket, and the included hotkeys.cpp uses this program to define mapping
between keys and these functions. The config file ".kwmrc" also uses *Kwmc* to apply initial settings upon startup.  
Because of this, *Kwmc* needs to be placed in your path to ensure that this works as expected.  
For information, check the readme located within the *kwmc* folder.

*Kwm* uses the event taps API (Quartz Event Services) to observe, filter and alter user input events
prior to their delivery to a foreground application.

This allows for functionality such as focus-follows-mouse, remapping keys, and most importantly 
global hotkeys, mainly to be used for interaction with the parts of *Kwm* that are exposed to hotkeys.cpp 
through the daemon, and so hotkeys.cpp can and should be customized by the user.  
The user may also use any other program that allows for running a specific command on keypress.

hotkeys.cpp can be edited and recompiled separately, thus any changes made does not require *Kwm* to be restarted.

Both autofocus and autoraise is available, however autofocus only redirects key input to the window below the cursor,
the menubar is not accessible. By default *Kwm* is set to use autoraise as it is meant to be used alongside
the tiling functionality, and so windows should not overlap unless a window is specifically made floating.


*EARLY multiple monitor support:*

Currently *Kwm* only looks for active monitors upon startup, and so if a monitor is connected or disconnected
while *Kwm* is running, it has to be restarted.
    
To send a window to the prev/next monitor, the hotkeys ctrl+alt+p/n can be used. 

Moving a window with the mouse WILL BREAK the window-trees of both monitors, however a window CAN be toggled
floating, which frees it from the tree. It can then be moved and made not floating on the other monitor.
This applies when moving a window between spaces on the same monitor as well.

NOTE: The API used to detect which monitor is active does not actually return the correct value unless
the monitor has been activated by the user. Simply moving the mouse over is not enough for the OSX API
to update. The user HAS TO CLICK somewhere on the new monitor after mousing oveer for the API to properly
return the expected value to *Kwm*. In addition to this, a lot of Mac Applications will start at the position
they were in previously, and will appear on the wrong monitor. This will most likely crash *Kwm* as of now.

*Kwm* requires access to osx accessibility.  Creating a certificate and codesigning the binary works as well.

Tested on Osx El Capitan (10.11.1 / 10.11.2).

![kwm mov](https://cloud.githubusercontent.com/assets/6175959/11498553/6ccedee6-9820-11e5-8830-96e0bf19b4f2.gif)

## Build:

To compile *Kwm*, simply run the included build.sh script.

By default, debug prints are enabled, and these can be turned off by opening the build.sh script and
getting rid of the -DDEBUG_BUILD flag.

If any changes have been made to hotkeys.cpp, run the sobuild.sh script to recompile this file separately.  
Because there is no app bundle, *Kwm* has to be started from a terminal.

In addition to this, for *Kwm* to work properly, the user also has to compile *Kwmc* and place it in the path.  
Simply run the build.sh script located in the *kwmc* folder, and move / symlink the binary.

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

## Configuration:

The default configuration file is `$HOME/.kwmrc` and is a script that contains *Kwmc* commands  
to be executed when *Kwm* starts.  
This file can be used to blacklist applications and specify other settings, as well as run any  
command not restricted to *Kwmc*.  

A sample config can be found within the [examples](examples) directory.

## Usage:

When *Kwm* starts, it will automatically tile the windows of the current space, using binary space partitioning.  
This will also happen once for any other space the user might switch to.

When *Kwm* detects a new window, it inserts it into a window tree at the specified point using the split-mode specified.  
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

The width of the left/right pane can be changed, as well as the height of the upper/lower pane.  
Currently these cannot occur simultaneously, and the type of panes created is decided by the main pane split-mode.

If the main pane is split using vertical mode, a left and right pane is created.  
If the main pane is split using horizontal mode, an upper and lower pane is created.  

## Default Hotkeys:
    - ctrl+alt+cmd:
        - t: toggle focus-mode (autofocus -> autoraise -> disabled)
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
