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
        - 7: set vertical split-mode
        - -: set horizontal split-mode

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

Currently does not create an app bundle.

To compile Kwm, simply run the included build.sh script.
By default, debug prints are enabled, and you can turn 
these off by opening the build.sh script and getting rid
of the -DDEBUG_BUILD flag.

Because there is no app bundle, Kwm has to be started from
a terminal. cd into the containing directory and type ./kwm

IF kwm is added to the path, a launcher script is necessary
because it tries to load hotkeys.so from the executables working-directory.

Example launcher script, that is placed in your path instead of the kwm binary.

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
spaces.plist is invalidated for some reason when your OSX 
enters sleep, and so Kwms windowing management often breaks
because of this. 

To fix this, you have to re-create all spaces and just drag
whatever applications you had opened, to the new spaces.
This causes OSX to update the spaces.plist file.
Restart Kwm and it should work again.
