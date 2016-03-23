######################################
# Example startup script.
#
# Activate space 1, and open iTerm2.
# Tell Kwm to restore bsp-layout used for development.
#
# Start rtorrent in the focused iTerm window, and
# open 4new iTerm windows and have them run some
# application.
#
# Switch to space 2 and open Firefox.
# Switch to space 3 and start Skype.

kwmc=/path/to/kwmc

$kwmc space -s id 1
sleep 1

open -na /Applications/iTerm2.app
sleep 1
$kwmc tree restore dev3
$kwmc write rtorrent
$kwmc press -return

$kwmc press cmd-n
$kwmc write vifm
$kwmc press -return

sleep 0.2
$kwmc press cmd-n

$kwmc press cmd-n
$kwmc write cd Documents/programming/C++/Kwm
$kwmc press -return
$kwmc write tmux
$kwmc press -return
$kwmc write vim
$kwmc press -return

$kwmc press cmd-n
$kwmc write cd Documents/programming/C++/Kwm
$kwmc press -return
$kwmc write tmux
$kwmc press -return

sleep 1
$kwmc space -s id 2
open -na /Applications/Firefox.app

sleep 1
$kwmc space -s id 3
open -na /Applications/Skype.app
