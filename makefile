DEBUG_BUILD   = -DDEBUG_BUILD -g
FRAMEWORKS    = -framework ApplicationServices -framework Carbon -framework Cocoa
XTRA_RPATH    = /Library/Developer/CommandLineTools/usr/lib/swift/macosx/
SDK_ROOT      = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
KWM_SRCS      = kwm/kwm.cpp kwm/tree.cpp kwm/window.cpp kwm/display.cpp kwm/daemon.cpp kwm/interpreter.cpp kwm/keys.cpp kwm/space.cpp kwm/border.cpp kwm/notifications.cpp kwm/workspace.mm
KWM_OBJS_TMP  = $(KWM_SRCS:.cpp=.o)
KWM_OBJS      = $(KWM_OBJS_TMP:.mm=.o)
KWMC_SRCS     = kwmc/kwmc.cpp kwmc/help.cpp
KWMC_OBJS     = $(KWMC_SRCS:.cpp=.o)
KWMO_SRCS     = kwm-overlay/kwm-overlay.swift kwm-overlay/Private-API.mm
KWMO_OBJS_TMP = $(KWMO_SRCS:.swift=.o)
KWMO_OBJS     = $(KWMO_OBJS_TMP:.mm=.o)
SAMPLE_CONFIG = examples/kwmrc
CONFIG_DIR    = $(HOME)/.kwm
BUILD_PATH    = ./bin
BUILD_FLAGS   = -O3 -Wall
BINS          = $(BUILD_PATH)/kwm $(BUILD_PATH)/kwmc $(BUILD_PATH)/kwm-overlay $(CONFIG_DIR)/kwmrc

all: $(BINS)

# The 'install' target forces a rebuild from clean with the DEBUG_BUILD
# variable clear so that we don't emit debug log messages.
install: DEBUG_BUILD=
install: clean $(BINS)

.PHONY: all clean install

# This is an order-only dependency so that we create the directory if it
# doesn't exist, but don't try to rebuild the binaries if they happen to
# be older than the directory's timestamp.
$(BINS): | $(BUILD_PATH)

$(BUILD_PATH):
	mkdir -p $(BUILD_PATH) && mkdir -p $(CONFIG_DIR)

clean:
	rm -rf $(BUILD_PATH)
	rm -f kwm/*.o
	rm -f kwmc/*.o
	rm -f kwm-overlay/*.o

$(BUILD_PATH)/kwm: $(KWM_OBJS)
	g++ $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -lpthread $(FRAMEWORKS) -o $@

kwm/%.o: kwm/%.cpp
	g++ -c $< $(DEBUG_BUILD) $(BUILD_FLAGS) -o $@

kwm/%.o: kwm/%.mm
	g++ -c $< $(DEBUG_BUILD) $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwmc: $(KWMC_OBJS)
	g++ $^ $(BUILD_FLAGS) -o $@

kwmc/%.o: kwmc/%.cpp
	g++ -c $< $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwm-overlay: $(KWMO_OBJS)
	swiftc $^ -o $@

kwm-overlay/%.o: kwm-overlay/%.swift
	swiftc -c $^ $(DEBUG_BUILD) -sdk $(SDK_ROOT) -import-objc-header kwm-overlay/Private-API.h -Xlinker -rpath -Xlinker $(XTRA_RPATH) -o $@

kwm-overlay/%.o: kwm-overlay/%.mm
	g++ -c $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwm_template.plist: $(KWM_PLIST)
	cp $^ $@

$(CONFIG_DIR)/kwmrc: $(SAMPLE_CONFIG)
	mkdir -p $(CONFIG_DIR)
	if test ! -e $@; then cp -n $^ $@; fi
