

RM=rm -f
MAKE=make

#CROSS_COMPILE=#aarch64-linux-gnu-
CROSS_COMPILE=arm-linux-gnueabihf-
GCC=$(CROSS_COMPILE)gcc 

INCLUDES=-I../lib
LOADDIR=-L../lib
LIBS=-lmath 
STATIC_LIBS=

DLL_FLAGS=-fPIC -shared
GCC_FLAGS=-g -DLED_NUM=2 -DDDP_NUM=2 -DOS_LINUX -DDEBUG -O2 -Wall -funsigned-char -Wno-unused-result

SYS_LIBS=-ldl -lm -lpthread

export OBJSDIR = $(shell pwd)

USART = bmctest
USARTOBJS = bmcMain.o usart.o sys_info.o utils.o linklist.o

################################################
$(USART): $(USARTOBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(STATIC_LIBS)
################################################
$(USARTOBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(LOADDIR) $(LIBS)
################################################

.PHONY : clean

clean:
	$(RM) *.o $(USART)

all:clean $(USART)

