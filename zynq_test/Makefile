

RM=rm -f
MAKE=make

CROSS_COMPILE=arm-linux-gnueabihf-
GCC=$(CROSS_COMPILE)g++

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

TARGET = fpga

OBJS=fpga.o 


################################################
$(TARGET): $(OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)
################################################
$(OBJS):%.o:%.cpp
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)
################################################
.PHONY : clean

clean:
	$(RM) *.o  $(TARGET) 

all:clean $(TARGET)
