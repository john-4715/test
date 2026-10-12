# Makefile for perf_analyzer (multi-file version)
#
# Sources: main.cpp options.cpp string_utils.cpp symbol_rules.cpp dataset.cpp input_parser.cpp analyzer.cpp
#
# Common commands:
#   make                    build (Release)
#   make debug              build debug version (-O0 -g, with address sanitizer)
#   make clean              clean
#   make install            install to $(PREFIX)/bin (default /usr/local/bin)
#   make report DATA=perf.data SYMFS=$$HOME/symfs_e3 KALLSYMS=kallsyms DURATION=120
#                           one command: perf script -> analysis report -> flame graph
#
# Cross compiling (usually unnecessary, analysis runs on the host; CROSS_COMPILE from the environment is not inherited):
#   make CROSS_COMPILE=aarch64-linux-gnu- LDFLAGS=-static

TARGET        := perf_analyzer
SRCS          := main.cpp options.cpp string_utils.cpp symbol_rules.cpp \
                 dataset.cpp input_parser.cpp analyzer.cpp
BUILD_DIR     := build
OBJS          := $(addprefix $(BUILD_DIR)/,$(SRCS:.cpp=.o))
DEBUG_OBJS    := $(addprefix $(BUILD_DIR)/debug/,$(SRCS:.cpp=.o))
DEPS          := $(OBJS:.o=.d) $(DEBUG_OBJS:.o=.d)

CROSS_COMPILE :=
CXX           := $(CROSS_COMPILE)g++
CXXSTD        ?= -std=c++17
CXXFLAGS      ?= -O2
WARNFLAGS     := -Wall -Wextra
LDFLAGS       ?=
PREFIX        ?= /usr/local

# ---- Parameters used by the report target ----
DATA          ?= perf.data
SYMFS         ?= $(HOME)/symfs_e3
KALLSYMS      ?= kallsyms
DURATION      ?= 0
TITLE         ?= ApplicationServices
FLAMEGRAPH    ?= $(HOME)/FlameGraph
PERF          ?= perf
PERF_SCRIPT_FLAGS ?= --force   # allow reading perf.data owned by another user (needed when run as root)
OUT_DIR       ?= report

.PHONY: all release debug clean install uninstall report help

all: release

release: $(BUILD_DIR)/$(TARGET)
	@ln -sf $(BUILD_DIR)/$(TARGET) $(TARGET)
	@echo "Build complete: ./$(TARGET)"

# Link
$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile: -MMD -MP generate header dependencies automatically, so editing a .h only rebuilds the affected .cpp files
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(WARNFLAGS) -MMD -MP -c -o $@ $<

# Debug build: separate object directory, does not affect the release build
DEBUG_FLAGS   := -O0 -g -fsanitize=address,undefined

debug: $(BUILD_DIR)/$(TARGET)_debug
	@echo "Debug build: $(BUILD_DIR)/$(TARGET)_debug"

$(BUILD_DIR)/$(TARGET)_debug: $(DEBUG_OBJS)
	$(CXX) -o $@ $^ -fsanitize=address,undefined $(LDFLAGS)

$(BUILD_DIR)/debug/%.o: %.cpp | $(BUILD_DIR)/debug
	$(CXX) $(CXXSTD) $(DEBUG_FLAGS) $(WARNFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR) $(BUILD_DIR)/debug:
	@mkdir -p $@

-include $(DEPS)

# One-step generation: perf script text, analysis report, folded stacks and flame graph
report: release
	@test -f "$(DATA)" || { echo "Capture data not found: $(DATA)"; exit 1; }
	@mkdir -p $(OUT_DIR)
	@echo "[1/3] perf script resolving symbols..."
	@KALL=""; [ -f "$(KALLSYMS)" ] && KALL="--kallsyms=$(KALLSYMS)"; \
	 SYM=""; [ -d "$(SYMFS)" ] && SYM="--symfs=$(SYMFS)"; \
	 $(PERF) script $(PERF_SCRIPT_FLAGS) -i $(DATA) $$SYM $$KALL > $(OUT_DIR)/out.perf 2> $(OUT_DIR)/perf_script.log || \
	 { echo "perf script failed, see $(OUT_DIR)/perf_script.log"; exit 1; }
	@echo "[2/3] Generating analysis report..."
	@./$(TARGET) $(OUT_DIR)/out.perf --duration $(DURATION) \
	    --folded-out $(OUT_DIR)/out.folded > $(OUT_DIR)/report.md
	@echo "[3/3] Generating flame graph..."
	@if [ -x "$(FLAMEGRAPH)/flamegraph.pl" ]; then \
	    $(FLAMEGRAPH)/flamegraph.pl --title "$(TITLE)" $(OUT_DIR)/out.folded > $(OUT_DIR)/flame.svg; \
	    echo "Flame graph: $(OUT_DIR)/flame.svg"; \
	 else \
	    echo "$(FLAMEGRAPH)/flamegraph.pl not found, skipping flame graph (set FLAMEGRAPH=<path>)"; \
	 fi
	@echo "Analysis report: $(OUT_DIR)/report.md"

install: release
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BUILD_DIR)/$(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

help:
	@echo "make                 build"
	@echo "make debug           build debug version"
	@echo "make clean           clean"
	@echo "make install         install to $(PREFIX)/bin"
	@echo "make report DATA=perf.data SYMFS=<symbol dir> KALLSYMS=<kallsyms> DURATION=<sec>"
	@echo "                     generates $(OUT_DIR)/report.md and $(OUT_DIR)/flame.svg"
