## Kwm

Started as a simple project to get true focus-follows-mouse support on OSX through event tapping.

### Features:
- Enables focus-follows-mouse
- Automatically tile windows as they are opened/destroyed
- System wide hotkeys (supports instantaneous live code editing)

### Requirements:
- Access to osx accessibility

### Hotkeys:
    - ctrl+alt+cmd:
        - t: toggle focus mode (focus-follows-mouse -> autoraise -> disabled)
        - r: rebuild hotkeys.cpp
        - q: restart kwm

        - h: decrease width
        - l: increase width
        - j: increase height
        - k: decrease height

        - f: toggle fullscreen for focused window
        - enter: opens a new iTerm window

        YTD player controls:
        - x: toggle play/pause
        - v: stop player

        - z: play previous video
        - c: play next video

        - a: increase volume
        - d: decrease volume

        - s: seek forward

    - ctrl+alt:
        - h: move window left
        - l: move window right
        - j: move window down
        - k: move window up

        - p: send window to previous screen
        - n: send window to next screen

    - alt+cmd
        - h: focus previous window in layout
        - l: focus next window in layout

        - p: swap with the previous window in layout
        - n: swap with the next window in layout
