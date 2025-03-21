

RM=rm -f
MAKE=make

CROSS_COMPILE=#loongarch64-linux-gnu-
GCC=$(CROSS_COMPILE)gcc

CP=cp
SOURCE=source


VERSION="const char version[]=\"version: 0.0.0.1\""
BUILDTIME="const char buildtime[]=\"buildtime: `date +%Y-%-m-%d-%H:%M`\""

INCLUDES=-I./lib
LOADDIR=-L./lib
LIBS=-lmath #-lLibPrb0400Drv 
STATIC_LIBS= #lib/libLibPrb0400Drv_dbg.a 

DLL_FLAGS=-fPIC -shared
GCC_FLAGS=-g -DLED_NUM=2 -DDDP_NUM=2 -DOS_LINUX -DDEBUG -O2 -Wall -funsigned-char

SYS_LIBS=-ldl -lm -lpthread

export OBJSDIR = $(shell pwd)

TARGET = spi_flash

OBJS = spi_flash.o main.o

$(OBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(LOADDIR) $(LIBS)

################################################
$(TARGET): $(OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(STATIC_LIBS)

################################################
clean:
	$(RM) *.o $(TARGET)

all:clean $(TARGET)

