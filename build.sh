#!/bin/bash
g++ hotkeys.cpp -shared -o hotkeys.so
g++ kwm.cpp tree.cpp window.cpp space.cpp display.cpp -DDEBUG_BUILD -o kwm -framework ApplicationServices -framework Carbon
