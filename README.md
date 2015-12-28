## Description (TL;DR)

[*Kwm*](https://koekeishiya.github.io/kwm) started as a simple project to get true focus-follows-mouse support on OSX through event tapping.  
It is now a tiling window manager that represents windows as the leaves of a binary tree.

*Kwm* runs a local daemon to read messages and trigger functions.  
*Kwmc* can be used to write to *Kwm*'s socket, and the included hotkeys.cpp uses this program to define  
a mapping between keys and these functions. For information, check the readme located within the *kwmc* folder.  

*Kwm* uses the event taps API (Quartz Event Services) to observe, filter and alter user input events prior  
to their delivery to a foreground application. This allows for functionality such as focus-follows-mouse,  
remapping keys, and most importantly global hotkeys.  

*Kwm* requires access to osx accessibility.  Creating a certificate and codesigning the binary works as well.  
Tested on Osx El Capitan (10.11.1 / 10.11.2).

       Screencast          |       Screenshot          |       Screenshot
:-------------------------:|:-------------------------:|:-------------------------:
![mov](https://cloud.githubusercontent.com/assets/6175959/11498553/6ccedee6-9820-11e5-8830-96e0bf19b4f2.gif)  |  ![img](https://cloud.githubusercontent.com/assets/6175959/12011903/1eeb3d6a-ace2-11e5-9c49-0df721e6e83d.png)  |  ![img](https://cloud.githubusercontent.com/assets/6175959/12011916/c8ef1b9c-ace2-11e5-9a5b-0aee3d41347a.jpg)

## Extended Information:

The different features; binary space partitioning, focus-follows-mouse and hotkey-system can all be enabled  
independently. This allows the user to choose what functionality fits their specific workflow.  

*Binary Space Partitioning:*  
Kwm tiles windows using a binary-tree structure. For information, check the usage section.  

*Focus-Follows-Mouse:*  
Both autofocus and autoraise is available, however autofocus only redirects key input to the window below the cursor,  
the menubar is not accessible. Autoraise gives a window focus and raises it to the front.  By default *Kwm* is set to  
use autoraise as it is meant to be used alongside the tiling functionality, and so windows should not overlap unless  
a window is specifically made floating.  

*System-Wide Hotkeys:*  
Kwm provides an instantaneous live-coding hotkey system for interacting with *Kwm*, and this can be customized  
by editing hotkeys.cpp. The user may use an external program for running a specific command on keypress instead.  
Using hotkeys to change window focus will work even if focus-follows-mouse has been disabled.  

*Multiple monitor support (in progress):*  
There are different ways to move a window between monitors.  
The first one is by using `kwmc screen -m prev|next|id`.  
The other option is to make the window floating and manually move it with the mouse, then un-float it.  
When moving a window directly with the mouse, *Kwm* will detect on mouse-release that the window  
position has changed, and automatically make it floating (Due to technical limitations this event  
must occur on the monitor which currently holds the window).

The first time a monitor is connected, the user may have to click several times on the screen for it to register.  
After this step, moving the mouse to a different monitor should activate the monitor automatically.  

## Build:

Because there is no app bundle, *Kwm* has to be started from a terminal.
To compile and run *Kwm*, simply run

      make
      ./bin/kwm

By default, debug prints are enabled, and these can be turned off by runnning

      make install

The hotkeys.cpp file can be edited live and recompiled separately using `make` again.  
By doing this, the user may change hotkeys without having to restart *Kwm*.  

To make *Kwm* start automatically on login, update line 9 of kwm.plist to be the full path of the executable  
on your machine. With that change, link the plist file `ln -sf full/path/to/kwm.plist ~/Library/LaunchAgents`.   
Then logout/login to have Kwm start automatically.  

This will also cause *Kwm* to restart itself when terminated, and no longer requires anything  
but the *Kwm* binary to have accessibility access.  

Removing the KeepAlive option on line 13-14 prevents *Kwm* from restarting once it is killed manually.  

## Configuration:

The default configuration file is `$HOME/.kwmrc` and is a script that contains *Kwmc* commands  
to be executed when *Kwm* starts. This file can be used to blacklist applications and specify  
a variety of settings, as well as run any command not restricted to *Kwmc*.  

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
        - q: Quit Kwm

        - m: mark the container to use for next window split

        - o: use width/height ratio to determine optimal split (default)
        - 7: use vertical split-mode
        - -: use horizontal split-mode
        - s: toggle split-type of existing container

        - h: move vertical-splitter left (increase width of right-pane containers)
        - l: move vertical-splitter right (increase width of left-pane containers)
        - j: move horizontal-splitter down (increase height of upper-pane containers)
        - k: move horizontal-splitter up (increase height of lower-pane containers)

        - f: toggle window fullscreen
        - p: toggle window parent container
        - w: toggle window floating
        - enter: opens a new iTerm window

    - ctrl+alt:
        - p: send window to previous screen
        - n: send window to next screen

        - 1: send window to screen id 0
        - 2: send window to screen id 1
        - 3: send window to screen id 2

        - x: increase horizontal gap
        - y: increase vertical gap

        - larrow: increase screen padding-left 
        - rarrow: increase screen padding-right 
        - uarrow: increase screen padding-top 
        - darrow: increase screen padding-bottom 

    - alt+cmd
        - t: toggle tiling mode of current space (float|tile)
        - r: rotate window-tree by 180 degrees

        - h: focus previous window
        - l: focus next window

        - p: swap with the previous window
        - n: swap with the next window
        - m: swap with the marked window

        - x: decrease horizontal gap
        - y: decrease vertical gap

        - larrow: decrease screen padding-left 
        - rarrow: decrease screen padding-right 
        - uarrow: decrease screen padding-top 
        - darrow: decrease screen padding-bottom 
