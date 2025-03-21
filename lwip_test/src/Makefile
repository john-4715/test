

RM=rm -f
MAKE=make

CROSS_COMPILE=#loongarch64-linux-gnu-
GCC=$(CROSS_COMPILE)gcc
GCC_PLUS=$(CROSS_COMPILE)g++

CP=cp
SOURCE=source

INCLUDES=-I./ -I../include  -I../contrib -I../contrib/ports/unix/port/include 
LOADDIR=-L/usr/lib -L../lib 

LIBS = 
#STATIC_LIBS = ../lib/liblwipcore.a  ../lib/liblwipcontribexamples.a  ../lib/liblwipcontribapps.a  ../lib/liblwipcontribaddons.a \
 	../lib/liblwipallapps.a ../lib/liblwipcontribportunix.a  ../lib/liblwipmbedtls.a 

STATIC_LIBS = ../lib/liblwip.a  
DLL_FLAGS=-fPIC -shared 

GCC_FLAGS=-g -DLED_NUM=2 -DDDP_NUM=2 -DOS_LINUX -DDEBUG -O2 -Wall -funsigned-char -Wno-error=deprecated-declarations \
	-Wno-deprecated-declarations 

SYS_LIBS=-ldl -lm -lpthread -pthread

export OBJSDIR = $(shell pwd)

LWIP_UDP_SEND = lwip_udp_send
LWIP_UDP_RECV = lwip_udp_recv

UDPOBJS = lwip_udp_send.o 

################################################
$(LWIP_UDP_SEND): $(UDPOBJS)
	$(GCC) -o $@ $? $(GCC_FLAGS) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS) 

################################################
$(UDPOBJS):%.o:%.c
	$(GCC) -c $< -o $@ $(GCC_FLAGS) $(INCLUDES) $(SYS_LIBS) $(LOADDIR) $(STATIC_LIBS) $(LIBS) 

################################################
.PHONY : clean

clean:
	$(RM) *.o  $(LWIP_UDP_SEND) 

all:clean $(LWIP_UDP_SEND)

