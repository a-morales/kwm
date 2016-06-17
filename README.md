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
For a list of available commands, [view the Kwmc configuration reference.](https://koekeishiya.github.io/kwm/kwmc.html)

For in depth information, [view the Kwm reference page.](https://koekeishiya.github.io/kwm)
For sample configurations and other useful scripts, check out the [wiki](https://github.com/koekeishiya/kwm/wiki).
You can also drop by the channel **##kwm** on [freenode](http://webchat.freenode.net)

*Kwm* requires access to the OSX accessibility API.
Tested on El Capitan (10.11.1 - 10.11.5).

![img](https://cloud.githubusercontent.com/assets/6175959/16150759/6822282e-3499-11e6-9a2e-5c61af2caba7.png)
For more screenshots, [click here.](https://github.com/koekeishiya/kwm/issues/2)

The bar seen in the above screenshot can be found [here](https://github.com/koekeishiya/nerdbar.widget).

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
to be executed when *Kwm* starts.  In addition to the above, the file `$HOME/.kwm/init` is a shell
script that will be ran after *Kwm* has finished setting up the internal configuration.

A sample config file can be found within the [examples](examples) directory.
Any error that occur during parsing of the config file will be written to **stderr**.
[Click here](https://github.com/koekeishiya/kwm/issues/285#issuecomment-216703278) for more information.

### Donate
First of all, *Kwm* will always be free and open source, however some users have
expressed interest in some way to show their support.

If you wish to do so, I have set up a patreon [here](https://www.patreon.com/aasvi).
