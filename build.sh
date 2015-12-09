#!/bin/bash
g++ hotkeys.cpp -DDEBUG_BUILD -shared -o hotkeys.so
g++ kwm.cpp tree.cpp window.cpp display.cpp daemon.cpp -DDEBUG_BUILD -o kwm -lpthread -framework ApplicationServices -framework Carbon
