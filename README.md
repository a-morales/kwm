## Kwm

Started as a simple project to get true focus-follows-mouse support on OSX through event tapping.
Scroll down for instructions on how to use.

### Features:
- Enables focus-follows-mouse
- Binary space partitioning window management
- System wide hotkeys (supports instantaneous live code editing)

### Requirements:
- Access to osx accessibility

### Hotkeys:
    - ctrl+alt+cmd:
        - t: toggle focus mode (focus-follows-mouse -> autoraise -> disabled)
        - r: rebuild hotkeys.cpp
        - q: restart kwm

        - m: mark the container to use for next window split
        - 7: set vertical split-mode
        - -: set horizontal split-mode

        - h: decrease width
        - l: increase width
        - j: increase height
        - k: decrease height

        - f: toggle window fullscreen
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
        - h: move window left
        - l: move window right
        - j: move window down
        - k: move window up

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
    ```
    pushd /path/to/kwm
    ./kwm
    popd
    ```

    The reason for this is that hotkeys.cpp can edited and rebuild separately,
    and Kwm will reload this library without having to be restarted and so
    hotkeys can be edited live.

    NOTE:
    Kwm reads com.apple.spaces.plist upon startup.
    spaces.plist is invalidated for some reason when your OSX 
    enters sleep, and so Kwms windowing management often breaks
    because of this. 
    
    To fix this, you have to re-create all spaces and just drag
    whatever applications you had opened, to the new spaces.
    This causes OSX to update the spaces.plist file and Kwm will work.
