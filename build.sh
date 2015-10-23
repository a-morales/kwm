#!/bin/bash
g++ kwm.cpp tree.cpp window.cpp space.cpp display.cpp hotkeys.cpp -DDEBUG_BUILD -o kwm -framework ApplicationServices -framework Carbon
