#!/bin/bash
g++ kwm.cpp tree.cpp window.cpp display.cpp hotkeys.cpp -DDEBUG_BUILD -o kwm -framework ApplicationServices -framework Carbon
