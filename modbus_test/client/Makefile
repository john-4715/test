#CC=arm-buildroot-linux-gnueabihf-gcc
CC=gcc
CFLAGS=-Wall -g -O2
TARGET=client

LDFLAGS=
IFLAGS=
LIB=

OBJS = client.o 
$(TARGET): $(OBJS)
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB)

%.o: %.c
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^ $(LDFLAGS) $(LIB)

clean:
	rm -f *.o $(TARGET) 
