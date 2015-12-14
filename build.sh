#!/bin/bash
g++ hotkeys.cpp -DDEBUG_BUILD -shared -O3 -o hotkeys.so
g++ kwm.cpp tree.cpp window.cpp display.cpp daemon.cpp -DDEBUG_BUILD -O3 -o kwm -lpthread -framework ApplicationServices -framework Carbon
