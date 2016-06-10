DEBUG_BUILD   = -DDEBUG_BUILD -g
FRAMEWORKS    = -framework ApplicationServices -framework Carbon -framework Cocoa
DEVELOPER_DIR = $(shell xcode-select -p)
SWIFT_STATIC  = $(DEVELOPER_DIR)/Toolchains/XcodeDefault.xctoolchain/usr/lib/swift_static/macosx
SDK_ROOT      = $(DEVELOPER_DIR)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
KWM_SRCS      = kwm/kwm.cpp kwm/container.cpp kwm/node.cpp kwm/tree.cpp kwm/window.cpp kwm/application.cpp kwm/display.cpp \
				kwm/daemon.cpp kwm/interpreter.cpp kwm/keys.cpp kwm/space.cpp kwm/border.cpp kwm/workspace.mm \
				kwm/serializer.cpp kwm/tokenizer.cpp kwm/rules.cpp kwm/scratchpad.cpp kwm/command.cpp kwm/config.cpp \
				kwm/axlib/axlib.cpp kwm/axlib/element.cpp kwm/axlib/window.cpp kwm/axlib/application.cpp kwm/axlib/observer.cpp \
				kwm/axlib/event.cpp kwm/axlib/sharedworkspace.mm kwm/axlib/display.mm kwm/axlib/carbon.cpp
KWM_OBJS_TMP  = $(KWM_SRCS:.cpp=.o)
KWM_OBJS      = $(KWM_OBJS_TMP:.mm=.o)
KWMC_SRCS     = kwmc/kwmc.cpp
KWMC_OBJS     = $(KWMC_SRCS:.cpp=.o)
KWMO_SRCS     = kwm-overlay/kwm-overlay.swift
KWMO_OBJS_TMP = $(KWMO_SRCS:.swift=.o)
KWMO_OBJS     = $(KWMO_OBJS_TMP:.mm=.o)
OBJS_DIR      = ./obj
SAMPLE_CONFIG = examples/kwmrc
CONFIG_DIR    = $(HOME)/.kwm
BUILD_PATH    = ./bin
BUILD_FLAGS   = -Wall
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
	rm -rf $(OBJS_DIR)

$(BUILD_PATH)/kwm: $(foreach obj,$(KWM_OBJS),$(OBJS_DIR)/$(obj))
	g++ $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -lpthread $(FRAMEWORKS) -o $@

$(OBJS_DIR)/kwm/%.o: kwm/%.cpp
	@mkdir -p $(@D)
	g++ -c $< $(DEBUG_BUILD) $(BUILD_FLAGS) -o $@

$(OBJS_DIR)/kwm/%.o: kwm/%.mm
	@mkdir -p $(@D)
	g++ -c $< $(DEBUG_BUILD) $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwmc: $(foreach obj,$(KWMC_OBJS),$(OBJS_DIR)/$(obj))
	g++ $^ $(BUILD_FLAGS) -o $@

$(OBJS_DIR)/kwmc/%.o: kwmc/%.cpp
	@mkdir -p $(@D)
	g++ -c $< $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwm-overlay: $(foreach obj,$(KWMO_OBJS),$(OBJS_DIR)/$(obj))
	swiftc $^ -lc++ -L $(SWIFT_STATIC) -Xlinker -force_load_swift_libs -o $@

$(OBJS_DIR)/kwm-overlay/%.o: kwm-overlay/%.swift
	@mkdir -p $(@D)
	swiftc -c $^ $(DEBUG_BUILD) -sdk $(SDK_ROOT) -o $@

$(OBJS_DIR)/kwm-overlay/%.o: kwm-overlay/%.mm
	@mkdir -p $(@D)
	g++ -c $^ $(DEBUG_BUILD) $(BUILD_FLAGS) -o $@

$(BUILD_PATH)/kwm_template.plist: $(KWM_PLIST)
	cp $^ $@

$(CONFIG_DIR)/kwmrc: $(SAMPLE_CONFIG)
	mkdir -p $(CONFIG_DIR)
	if test ! -e $@; then cp -n $^ $@; fi
