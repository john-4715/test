

RM=rm -f
MAKE=make

CROSS_COMPILE=arm-linux-gnueabihf-
GCC=$(CROSS_COMPILE)gcc
GCC_PLUS=$(CROSS_COMPILE)g++

CP=cp
SOURCE=source


VERSION="const char version[]=\"version: 0.0.0.1\""
BUILDTIME="const char buildtime[]=\"buildtime: `date +%Y-%-m-%d-%H:%M`\""

INCLUDES=-I 
LOADDIR=-L 

LIBS=-lz 
STATIC_LIBS=
DLL_FLAGS=-fPIC -shared 
GCC_FLAGS=-g -DLED_NUM=2 -DDDP_NUM=2 -DOS_LINUX -DDEBUG -O2 -Wall -funsigned-char

SYS_LIBS=-ldl -lm -lpthread 

export OBJSDIR = $(shell pwd)

FPGAREAD = fpga_read
FPGAWRITE = fpga_write
SERIAL_SEND = serial_send
SERIAL_RECV = serial_recv
IRQ_TEST = irq_test
AUDIO_SEND = audio_send

OBJS=fpga_read.o
WRITEOBJS=fpag_write.o
SSEND_OBJS=serial_send.o
SRECV_OBJS=serial_recv.o
IRQ_OBJS=irq_test.o
AUDIO_OBJS=audio_send.o

################################################
$(FPGAREAD): $(OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(FPGAWRITE): $(WRITEOBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(SERIAL_SEND): $(SSEND_OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(SERIAL_RECV): $(SRECV_OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)	

$(IRQ_TEST): $(IRQ_OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)	

$(AUDIO_SEND): $(AUDIO_OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)	
################################################
$(OBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(WRITEOBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(SSEND_OBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(SRECV_OBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(IRQ_OBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(AUDIO_OBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)
################################################
.PHONY : clean

clean:
	$(RM) *.o  $(FPGAREAD) $(FPGAWRITE) $(SERIAL_SEND) $(SERIAL_RECV) $(IRQ_TEST) $(AUDIO_SEND)

all:clean $(FPGAREAD) $(FPGAWRITE) $(SERIAL_SEND) $(SERIAL_RECV) $(IRQ_TEST) $(AUDIO_SEND)

