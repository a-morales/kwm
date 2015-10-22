## kwm

Started as a simple project to get true focus-follows-mouse support on OSX through event tapping.

### Features:
- Enables focus-follows-mouse
- Automatically tile windows as they are opened/destroyed
- System wide hotkeys

### Requirements:
- Access to osx accessibility

### hotkeys:
    - ctrl+alt+cmd:
        - t: toggle focus mode (focus-follows-mouse -> autoraise -> disabled)

        - h: decrease width
        - l: increase width
        - j: increase height
        - k: decrease height

        - enter: opens a new iTerm window
        - f: toggle fullscreen for focused window

        YTD player controls:
        - z: play previous video
        - x: toggle play/pause
        - c: play next video
        - v: stop player

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
