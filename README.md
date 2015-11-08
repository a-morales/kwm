## Kwm

Started as a simple project to get true focus-follows-mouse support on OSX through event tapping.

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
