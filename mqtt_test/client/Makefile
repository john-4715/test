

RM=rm -f
MAKE=make

CROSS_COMPILE=
GCC=$(CROSS_COMPILE)gcc
GCC_PLUS=$(CROSS_COMPILE)g++

CP=cp
SOURCE=source


VERSION="const char version[]=\"version: 0.0.0.1\""
BUILDTIME="const char buildtime[]=\"buildtime: `date +%Y-%-m-%d-%H:%M`\""

INCLUDES=-I/usr/include
LOADDIR=-L 

LIBS=-lz 
STATIC_LIBS=
DLL_FLAGS=-fPIC -shared 
GCC_FLAGS=-g -DLED_NUM=2 -DDDP_NUM=2 -DOS_LINUX -DDEBUG -O2 -Wall -funsigned-char -lmosquitto

SYS_LIBS=-ldl -lm -lpthread 

export OBJSDIR = $(shell pwd)

PUBLISH = pub
SUBSCRIBE = sub

PUBOBJS = pub.o
SUBOBJS = sub.o

################################################
$(PUBLISH): $(PUBOBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(SUBSCRIBE): $(SUBOBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)
################################################
$(PUBOBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)

$(SUBOBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS)
################################################
.PHONY : clean

clean:
	$(RM) *.o  $(PUBLISH) $(SUBSCRIBE)

all:clean $(PUBLISH) $(SUBSCRIBE)
