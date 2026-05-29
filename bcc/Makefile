# ARM 32bit buildroot 编译器
CC      = arm-buildroot-linux-gnueabihf-gcc
CFLAGS  = -O2 -Wall -static -s
LDFLAGS = -static

BINDIR := bin

# 全部 34 个工具
TARGETS := \
    $(BINDIR)/execsnoop \
    $(BINDIR)/opensnoop \
    $(BINDIR)/biosnoop \
    $(BINDIR)/bindsnoop \
    $(BINDIR)/tcpconnect \
    $(BINDIR)/statsnoop \
    $(BINDIR)/syscount \
    $(BINDIR)/biolatency \
    $(BINDIR)/filetop \
    $(BINDIR)/exitsnoop \
    $(BINDIR)/tcpconnlat \
    $(BINDIR)/softirqs \
    $(BINDIR)/hardirqs \
    $(BINDIR)/dirtop \
    $(BINDIR)/tcpstates \
    $(BINDIR)/tcptop \
    $(BINDIR)/tcprtt \
    $(BINDIR)/syncsnoop \
    $(BINDIR)/swapin \
    $(BINDIR)/mountsnoop \
    $(BINDIR)/oomkill \
    $(BINDIR)/threadsnoop \
    $(BINDIR)/trace \
    $(BINDIR)/tplist \
    $(BINDIR)/bashreadline \
    $(BINDIR)/capable \
    $(BINDIR)/cpudist \
    $(BINDIR)/runqlen \
    $(BINDIR)/runqlat \
    $(BINDIR)/tcplife \
    $(BINDIR)/tcpretrans \
    $(BINDIR)/wakeuptime \
    $(BINDIR)/pidpersec \
    $(BINDIR)/numasched

all: $(BINDIR) $(TARGETS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/%: %.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(BINDIR)

help:
	@echo "make        - 编译全部工具到 bin/"
	@echo "make clean  - 删除 bin 目录"