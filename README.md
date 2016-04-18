## Brief Description [![Build Status](https://travis-ci.org/koekeishiya/kwm.svg?branch=master)](https://travis-ci.org/koekeishiya/kwm)

**WARNING:** The master branch is considered bleeding edge and changes may break configurations at any time! Check the latest release for a stable version.

[*Kwm*](https://koekeishiya.github.io/kwm) started as a simple project to get true focus-follows-mouse support on OSX through event tapping.
It is now a tiling window manager that represents windows as the leaves of a binary tree.
*Kwm* supports binary space partitioned, monocle and floating spaces.

*Kwm* uses the event taps API (Quartz Event Services) to observe, filter and alter user input events prior
to their delivery to a foreground application. This allows for functionality such as focus-follows-mouse,
and different types of hotkeys.

*Kwm* runs a local daemon to read messages and trigger functions.
*Kwmc* is used to write to *Kwm*'s socket, and must be used when interacting with and configuring how *Kwm* works.
For a list of various commands that can be issued, check the readme located within the *Kwmc* folder.

For in depth information, sample configurations, and useful scripts to implement new features by combining
*Kwmc* commands, check out the [wiki](https://github.com/koekeishiya/kwm/wiki). You can also drop by the channel ##kwm** on [freenode](http://webchat.freenode.net)

*Kwm* requires access to the OSX accessibility API.
Tested on El Capitan (10.11.1 - 10.11.4).

![img](https://cloud.githubusercontent.com/assets/6175959/12651784/55c7debe-c5e8-11e5-836f-97f99f2b4529.png)
For more screenshots, [click here.](https://github.com/koekeishiya/kwm/issues/2)

[herrbischoff](https://github.com/herrbischoff) has re-created my bar from the screenshots as a widget
for [Übersicht](http://tracesof.net/uebersicht/) for those that would like to have a decent bar with their setup.
This widget can be found here: https://github.com/herrbischoff/nerdbar.widget

## Extended Information:

The different features; binary space partitioning, focus-follows-mouse and the hotkey-system can all be enabled
independently. This allows the user to choose which functionality to enable depending on their workflow.

*Binary Space Partitioning:*
*Kwm* tiles windows using a binary-tree structure. For information, check the usage section.

*Focus-Follows-Mouse:*
Both autofocus and autoraise is available, however autofocus only redirects key input to the window below the cursor,
the menubar is not accessible. Autoraise gives a window focus and raises it to the front.  By default *Kwm* is set to
use autoraise as it is meant to be used alongside the tiling functionality, and so windows should not overlap unless
a window is specifically made floating.

*System-Wide Hotkeys:*
*Kwm* allows the user to bind and unbind hotkeys to commands through the *Kwmc* tool, using a bind and unbind option.
These binds support the use of a single prefix, which may be bind-specific or global (apply to all binds).
There are 3 types of hotkeys: global, global + blacklist applications, specified applications.
Using hotkeys to change window focus will work even if focus-follows-mouse has been disabled.

*Multiple monitor support:*
*Kwm* supports external monitors and have commands that allow for focusing screens, and moving windows.

## Install:

A codesigned binary release is available through Homebrew

      brew install homebrew/binary/kwm

To compile from source

      make install

Build with debug information

      make

To make *Kwm* start automatically on login through launchd, if compiled from source

      edit /path/to/kwm on line 9 of examples/com.koekeishiya.kwm.plist
      cp examples/com.koekeishiya.kwm.plist ~/Library/LaunchAgents/

I would recommend for *Kwm* to be managed by launchd, as it otherwise requires
the terminal application to have Accessibility Permissions (Not Recommended).

## Configuration:

The default configuration file is `$HOME/.kwm/kwmrc` and is a script that contains *Kwmc* commands
to be executed when *Kwm* starts. This file can be used to blacklist applications and specify
a variety of settings, as well as run any command not restricted to *Kwmc*.

*Kwm* can apply all of these settings during runtime, and so live testing of options is possible
before writing them into the config file.

In addition to the above, the file `$HOME/.kwm/init` is a shell script that will be ran after
*Kwm* has finished setting up the internal configuration.

A sample config file can be found within the [examples](examples) directory.

**Note:** The sample configuration and information is updated to follow *Kwm* @ master. If you are running a release
version, check the documentation provided at that particular tag.

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

A node in the bsp-tree can be in 2 different states:
 - Tree Node
 - Link Node

Example of a bsp-tree consisting of only Tree Nodes:

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

Example of a bsp-tree consisting of a [monocle-subtree (a Link Node)](https://github.com/koekeishiya/kwm/pull/231):

```
          a                       a                       a
         / \         -->         / \         -->         / \
        1   b                   1   b                   1   b
           / \                 /   / \                 /   / \
          3   2               4   3   2               4   3   2
                                                     /
                                                    5

---------------------   ---------------------   ---------------------
| SubTree |         |   | SubTree |         |   | SubTree |         |
|    1    |    3    |   |    1    |    3    |   |    1    |    3    |
|         |         |   |    |    |         |   |    |    |         |
|         |---------|   |    4    |---------|   |    4    |---------|
|         |         |   |         |         |   |    |    |         |
|         |    2    |   |         |    2    |   |    5    |    2    |
|         |         |   |         |         |   |         |         |
---------------------   ---------------------   ---------------------

```

In addition to bsp, *Kwm* supports both monocle and floating spaces.
If a space is set to floating mode, *Kwm* will ignore this space.
If a space is in monocle mode, every window will run fullscreen.

If a window is not detected by *Kwm*, it is most likely due to a 'window role' mismatch.
Use the command `kwmc config add-role role application` to fix this.
See https://github.com/koekeishiya/kwm/issues/40 for information.

### Donate
First of all, *Kwm* will always be free and open source, however some users have
expressed interest in some way to show their support.

If you wish to do so, I have set up a patreon [here](https://www.patreon.com/aasvi).
