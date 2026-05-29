CC=arm-buildroot-linux-gnueabihf-gcc
#CC=gcc
CFLAGS=-Wall -g -Wextra -O2 -lm
TARGET=sys_monitor

LDFLAGS=
IFLAGS=
LIB=

OBJS = main.o 
$(TARGET): $(OBJS)
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB)

%.o: %.c
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^ $(LDFLAGS) $(LIB)

clean:
	rm -f *.o $(TARGET) 
