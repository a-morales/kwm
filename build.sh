#!/bin/bash
g++ kwm.cpp window.cpp display.cpp spaces.cpp layout.cpp hotkeys.cpp -DDEBUG_BUILD -o kwm -framework ApplicationServices -framework Carbon
