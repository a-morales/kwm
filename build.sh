#!/bin/bash
g++ kwm.cpp window.cpp display.cpp spaces.cpp layout.cpp hotkeys.cpp -o kwm -framework ApplicationServices -framework Carbon
