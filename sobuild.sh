#!/bin/bash
g++ hotkeys.cpp -shared -DDEBUG_BUILD -O3 -o hotkeys.so
