DEBUG_BUILD=-DDEBUG_BUILD
FRAMEWORKS=-framework ApplicationServices -framework Carbon -framework Cocoa
SOURCE_FILES=kwm/kwm.cpp kwm/tree.cpp kwm/window.cpp kwm/display.cpp kwm/daemon.cpp
BUILD_PATH=./bin
BUILD_FLAGS=-O3 -lpthread -o $(BUILD_PATH)/kwm

.PHONY: kwmc

all: kwm kwmc

make_bin:
	mkdir -p $(BUILD_PATH)

clean:
	rm -rf $(BUILD_PATH)

hotkeys: make_bin
	g++ ./kwm/hotkeys.cpp $(DEBUG_BUILD) -O3 -shared -o $(BUILD_PATH)/hotkeys.so

kwm: hotkeys make_bin
	g++ $(SOURCE_FILES) $(DEBUG_BUILD) $(BUILD_FLAGS) $(FRAMEWORKS)

kwmc: make_bin
	g++ ./kwmc/kwmc.cpp -o $(BUILD_PATH)/kwmc

install: hotkeys make_bin
	g++ $(SOURCE_FILES) $(BUILD_FLAGS) $(FRAMEWORKS)
