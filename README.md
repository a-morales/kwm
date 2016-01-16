## Brief Description

[*Kwm*](https://koekeishiya.github.io/kwm) started as a simple project to get true focus-follows-mouse support on OSX through event tapping.  
It is now a tiling window manager that represents windows as the leaves of a binary tree.  
*Kwm* supports binary space partitioned, monocle and floating spaces.  

*Kwm* uses the event taps API (Quartz Event Services) to observe, filter and alter user input events prior  
to their delivery to a foreground application. This allows for functionality such as focus-follows-mouse,  
remapping keys, and most importantly global hotkeys.  

*Kwm* runs a local daemon to read messages and trigger functions.  
*Kwmc* is used to write to *Kwm*'s socket, and must be used when interacting with and configuring how *Kwm* works.  
For a list of various commands that can be issued, check the readme located within the *kwmc* folder.  
The *Kwmc* tool also has a built-in help system that can be accessed from the terminal using `kwmc help`.  

*Kwm* requires access to the OSX accessibility API.  
Tested on El Capitan (10.11.1 / 10.11.2).

![img](https://cloud.githubusercontent.com/assets/6175959/12092967/8d8853d8-b300-11e5-8a44-ec1245efdc74.png)
For more screenshots, [click here.](https://github.com/koekeishiya/kwm/issues/2)  

[herrbischoff](https://github.com/herrbischoff) has re-created my bar from the screenshots as a widget  
for [Übersicht](http://tracesof.net/uebersicht/) for those that would like to have a decent bar with their setup.  
This widget can be found here: https://github.com/herrbischoff/nerdbar.widget  

## Extended Information:

The different features; binary space partitioning, focus-follows-mouse and the hotkey-system can all be enabled  
independently. This allows the user to choose which functionality to enable depending on their workflow.  

*Binary Space Partitioning:*  
Kwm tiles windows using a binary-tree structure. For information, check the usage section.  

*Focus-Follows-Mouse:*  
Both autofocus and autoraise is available, however autofocus only redirects key input to the window below the cursor,  
the menubar is not accessible. Autoraise gives a window focus and raises it to the front.  By default *Kwm* is set to  
use autoraise as it is meant to be used alongside the tiling functionality, and so windows should not overlap unless  
a window is specifically made floating.  

*System-Wide Hotkeys:*  
Kwm allows the user to bind and unbind hotkeys to commands through the *Kwmc* tool, using a bind and unbind option.  
For more advanced use, there is also an instantaneous live-coding hotkey system and this can be customized by editing  
hotkeys.cpp. The user may use an external program for running a specific command on keypress instead.  
Using hotkeys to change window focus will work even if focus-follows-mouse has been disabled.  

*Multiple monitor support:*  
Kwm does support external monitors and have commands that allow for focusing screens,  moving windows, and capturing applications.
For more information, see `kwmc help screen` and `kwmc config capture`.  

## Build:

Because there is no app bundle, *Kwm* has to be started from a terminal.  
To compile and start *Kwm*, simply run

      make
      ./bin/kwm

By default, debug prints are enabled, and these can be turned off by running

      make install

The hotkeys.cpp file can be edited live and recompiled separately using `make` again.  
By doing this, the user may change hotkeys without having to restart *Kwm*.  

To make *Kwm* start automatically on login, run the following command the first time *Kwm* starts 

      kwmc config launchd enable

## Configuration:

The default configuration file is `$HOME/.kwm/kwmrc` and is a script that contains *Kwmc* commands  
to be executed when *Kwm* starts. This file can be used to blacklist applications and specify  
a variety of settings, as well as run any command not restricted to *Kwmc*.  

*Kwm* can apply all of these settings during runtime, and so live testing of options is possible  
before writing them into the config file.

A sample config can be found within the [examples](examples) directory.

## Usage:

When *Kwm* starts, it will automatically tile the windows of the current space using the tiling mode set in the config file.  
By default, it will use binary space partitioning. This will also happen once for any other space the user might switch to.

When *Kwm* detects a new window, it is inserted into a window tree using an insertion point, with the given split-mode.  
When a window is closed, it will be removed from the window tree and the tree will be rebalanced.  
By default, the insertion point is the focused container, but a temporary insertion point can be set.  

There are 3 types of split-modes available:
 - Optimal - uses width/height ratio (default)
 - Vertical
 - Horizontal  

Example:

```
          a                       a                       a
         / \         -->         / \         -->         / \    
        1   2                   1   b                   1   b
                                   / \                     / \
                                  3   2                   c   2
                                                         / \
                                                        4   3

---------------------   ---------------------   --------------------- 
|         |         |   |         |         |   |         |    |    |
|         |         |   |         |    3    |   |         | 4  |  3 |
|         |         |   |         |    *    |   |         | *  |    |
|    1    |    2    |   |    1    |---------|   |    1    |---------|
|         |    *    |   |         |         |   |         |         |
|         |         |   |         |    2    |   |         |    2    |
|         |         |   |         |         |   |         |         |
---------------------   ---------------------   ---------------------

```

By taking advantage of the fact that bsp-trees can be saved and restored  
from file, it is trivial to create layouts that can be loaded on the press  
of a hotkey. This allows the user to quickly get a given space to tile  
according to their needs as they change through the day.  
To save/restore a bsp-layout, see `kwmc help tree`.  

In addition to bsp, *Kwm* supports both monocle and floating spaces.  
If a space is set to floating mode, nothing will be tiled for this space.  
If a space is in monocle mode, every window will run fullscreen, and the  
user can switch between open windows using the kwmc command `window -f prev|next`.  

If a window is not detected by Kwm, it is most likely due to a 'window role' mismatch.  
Use the command `kwmc config add-role role application` to fix this.  
See https://github.com/koekeishiya/kwm/issues/40 for information.  
