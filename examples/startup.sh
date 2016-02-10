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

/usr/local/bin/kwmc space -s id 1
sleep 1

open -na /Applications/iTerm2.app
sleep 1
/usr/local/bin/kwmc tree restore dev3
/usr/local/bin/kwmc write rtorrent
/usr/local/bin/kwmc press -return

/usr/local/bin/kwmc press cmd-n
/usr/local/bin/kwmc write vifm
/usr/local/bin/kwmc press -return

sleep 0.2
/usr/local/bin/kwmc press cmd-n

/usr/local/bin/kwmc press cmd-n
/usr/local/bin/kwmc write cd Documents/programming/C++/Kwm
/usr/local/bin/kwmc press -return
/usr/local/bin/kwmc write tmux
/usr/local/bin/kwmc press -return
/usr/local/bin/kwmc write vim
/usr/local/bin/kwmc press -return

/usr/local/bin/kwmc press cmd-n
/usr/local/bin/kwmc write cd Documents/programming/C++/Kwm
/usr/local/bin/kwmc press -return
/usr/local/bin/kwmc write tmux
/usr/local/bin/kwmc press -return

sleep 1
/usr/local/bin/kwmc space -s id 2
open -na /Applications/Firefox.app

sleep 1
/usr/local/bin/kwmc space -s id 3
open -na /Applications/Skype.app
