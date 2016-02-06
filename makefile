DEBUG_BUILD=-DDEBUG_BUILD -g
FRAMEWORKS=-framework ApplicationServices -framework Carbon -framework Cocoa
SDK_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
KWM_SRCS=kwm/kwm.cpp kwm/tree.cpp kwm/window.cpp kwm/display.cpp kwm/daemon.cpp kwm/interpreter.cpp kwm/keys.cpp kwm/space.cpp kwm/border.cpp kwm/notifications.cpp kwm/helpers.cpp
KWMC_SRCS=kwmc/kwmc.cpp kwmc/help.cpp
KWMO_SRCS=kwm-overlay/kwm-overlay.swift
KWM_PLIST=kwm.plist
SAMPLE_CONFIG=examples/kwmrc
CONFIG_DIR=$(HOME)/.kwm
BUILD_PATH=./bin
BUILD_FLAGS=-O3 -Wall
BINS=$(BUILD_PATH)/kwm $(BUILD_PATH)/kwmc $(BUILD_PATH)/kwm-overlay $(BUILD_PATH)/kwm_template.plist $(CONFIG_DIR)/kwmrc

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

$(BUILD_PATH)/kwm: $(KWM_SRCS)
	g++ $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -lpthread $(FRAMEWORKS) -o $@

$(BUILD_PATH)/kwmc: $(KWMC_SRCS)
	g++ $^ $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwm-overlay: $(KWMO_SRCS)
	swiftc -sdk $(SDK_ROOT) $^ -o $@

$(BUILD_PATH)/kwm_template.plist: $(KWM_PLIST)
	cp $^ $@

$(CONFIG_DIR)/kwmrc: $(SAMPLE_CONFIG)
	mkdir -p $(CONFIG_DIR)
	if test ! -e $@; then cp -n $^ $@; fi
