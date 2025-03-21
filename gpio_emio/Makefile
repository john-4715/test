

RM=rm -f
MAKE=make

CROSS_COMPILE=arm-linux-gnueabihf-
GCC=$(CROSS_COMPILE)gcc
GCC_PLUS=$(CROSS_COMPILE)g++

CP=cp
SOURCE=source

INCLUDES=-I 
LOADDIR=-L 

LIBS=-lz 
STATIC_LIBS=
DLL_FLAGS=-fPIC -shared 
GCC_FLAGS=-g -DLED_NUM=2 -DDDP_NUM=2 -DOS_LINUX -DDEBUG -O2 -Wall -funsigned-char

SYS_LIBS=-ldl -lm -lpthread 

export OBJSDIR = $(shell pwd)

GPIO_EMIO = gpio_emio

OBJS=gpio_emio.o

################################################
$(GPIO_EMIO): $(OBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)
################################################
$(OBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)
################################################
.PHONY : clean

clean:
	$(RM) *.o  $(GPIO_EMIO) 

all:clean $(GPIO_EMIO) 
