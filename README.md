## kwm

Started as a simple project to get true focus-follows-mouse support on OSX through event tapping.

### hotkeys:
    - ctrl+alt+cmd:
        - t: toggle keypress redirect
        - r: toggle auto-raise

        - h: decrease width
        - l: increasee width
        - j: increasee height
        - k: decrease height

        - space: toggle screen layout

        - up: move and resize upper-half
        - down: move and resize lower-half
        - left: move and resize left-half
        - right: move and resize right-half

        - p: move and resize upper-left quarter
        - å: move and resize upper-right quarter
        - ø: move and resize lower-left quarter
        - æ: move and resize lower-right quarter
    
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

        - tab: cycle windows and place them
               in the focused layout

### Features:
    - Enables focus-follows-mouse
    - Assigning windows to predefined layouts
    - System wide hotkeys

### Requirements:
    - Access to osx accessibility
