# Makefile for perf_analyzer
#
# 常用命令：
#   make                    编译（Release）
#   make debug              编译调试版本（-O0 -g，带地址检查）
#   make clean              清理
#   make install            安装到 $(PREFIX)/bin（默认 /usr/local/bin）
#   make report DATA=perf.data SYMFS=$$HOME/symfs_e3 KALLSYMS=kallsyms DURATION=120
#                           一条命令完成：perf script -> 分析报告 -> 火焰图
#
# 交叉编译（一般不需要，分析在主机上进行；不会继承环境变量中的 CROSS_COMPILE）：
#   make CROSS_COMPILE=aarch64-linux-gnu- LDFLAGS=-static

TARGET        := perf_analyzer
SRC           := perf_analyzer.cpp
BUILD_DIR     := build

CROSS_COMPILE :=
CXX           := $(CROSS_COMPILE)g++
CXXSTD        ?= -std=c++17
CXXFLAGS      ?= -O2
WARNFLAGS     := -Wall -Wextra
LDFLAGS       ?=
PREFIX        ?= /usr/local

# ---- report 目标使用的参数 ----
DATA          ?= perf.data
SYMFS         ?= $(HOME)/symfs_e3
KALLSYMS      ?= kallsyms
DURATION      ?= 0
TITLE         ?= ApplicationServices
FLAMEGRAPH    ?= $(HOME)/FlameGraph
PERF          ?= perf
PERF_SCRIPT_FLAGS ?= --force   # 允许读取其他用户的 perf.data（root 下执行时需要）
OUT_DIR       ?= report

.PHONY: all release debug clean install uninstall report help

all: release

release: $(BUILD_DIR)/$(TARGET)
	@ln -sf $(BUILD_DIR)/$(TARGET) $(TARGET)
	@echo "编译完成: ./$(TARGET)"

$(BUILD_DIR)/$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(WARNFLAGS) -o $@ $< $(LDFLAGS)

debug: CXXFLAGS := -O0 -g -fsanitize=address,undefined
debug: LDFLAGS  += -fsanitize=address,undefined
debug: $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(WARNFLAGS) -o $(BUILD_DIR)/$(TARGET)_debug $< $(LDFLAGS)
	@echo "调试版本: $(BUILD_DIR)/$(TARGET)_debug"

$(BUILD_DIR):
	@mkdir -p $@

# 一键生成：perf script 文本、分析报告、折叠栈和火焰图
report: release
	@test -f "$(DATA)" || { echo "找不到采集数据: $(DATA)"; exit 1; }
	@mkdir -p $(OUT_DIR)
	@echo "[1/3] perf script 解析符号..."
	@KALL=""; [ -f "$(KALLSYMS)" ] && KALL="--kallsyms=$(KALLSYMS)"; \
	 SYM=""; [ -d "$(SYMFS)" ] && SYM="--symfs=$(SYMFS)"; \
	 $(PERF) script $(PERF_SCRIPT_FLAGS) -i $(DATA) $$SYM $$KALL > $(OUT_DIR)/out.perf 2> $(OUT_DIR)/perf_script.log || \
	 { echo "perf script 失败，详见 $(OUT_DIR)/perf_script.log"; exit 1; }
	@echo "[2/3] 生成分析报告..."
	@./$(TARGET) $(OUT_DIR)/out.perf --duration $(DURATION) \
	    --folded-out $(OUT_DIR)/out.folded > $(OUT_DIR)/report.md
	@echo "[3/3] 生成火焰图..."
	@if [ -x "$(FLAMEGRAPH)/flamegraph.pl" ]; then \
	    $(FLAMEGRAPH)/flamegraph.pl --title "$(TITLE)" $(OUT_DIR)/out.folded > $(OUT_DIR)/flame.svg; \
	    echo "火焰图: $(OUT_DIR)/flame.svg"; \
	 else \
	    echo "未找到 $(FLAMEGRAPH)/flamegraph.pl，跳过火焰图（可设置 FLAMEGRAPH=路径）"; \
	 fi
	@echo "分析报告: $(OUT_DIR)/report.md"

install: release
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BUILD_DIR)/$(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

help:
	@echo "make                 编译"
	@echo "make debug           编译调试版本"
	@echo "make clean           清理"
	@echo "make install         安装到 $(PREFIX)/bin"
	@echo "make report DATA=perf.data SYMFS=<符号目录> KALLSYMS=<kallsyms> DURATION=<秒>"
	@echo "                     生成 $(OUT_DIR)/report.md 和 $(OUT_DIR)/flame.svg"
