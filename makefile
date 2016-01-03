DEBUG_BUILD=-DDEBUG_BUILD
FRAMEWORKS=-framework ApplicationServices -framework Carbon -framework Cocoa
KWM_SRCS=kwm/kwm.cpp kwm/tree.cpp kwm/window.cpp kwm/display.cpp kwm/daemon.cpp kwm/interpreter.cpp kwm/keys.cpp
HOTKEYS_SRCS=kwm/hotkeys.cpp
KWMC_SRCS=kwmc/kwmc.cpp
KWM_PLIST=kwm.plist
SAMPLE_CONFIG=examples/kwmrc
BUILD_PATH=./bin
BUILD_FLAGS=-O3
BINS=$(BUILD_PATH)/hotkeys.so $(BUILD_PATH)/kwm $(BUILD_PATH)/kwmc $(BUILD_PATH)/kwm_template.plist $(HOME)/.kwmrc

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
	mkdir -p $(BUILD_PATH)

clean:
	rm -rf $(BUILD_PATH)

$(BUILD_PATH)/hotkeys.so: $(HOTKEYS_SRCS)
	g++ $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -shared -o $@

$(BUILD_PATH)/kwm: $(KWM_SRCS)
	g++ $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -lpthread $(FRAMEWORKS) -o $@

$(BUILD_PATH)/kwmc: $(KWMC_SRCS)
	g++ $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwm_template.plist: $(KWM_PLIST)
	cp $^ $@

$(HOME)/.kwmrc: $(SAMPLE_CONFIG)
	cp -n $^ $@
