#CC=arm-buildroot-linux-gnueabihf-g++
CC=g++
CFLAGS=-Wall -g -O2
TARGET=server

LDFLAGS=
IFLAGS=
LIB=

OBJS = server.o 
$(TARGET): $(OBJS)
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB)

%.o: %.cpp
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^ $(LDFLAGS) $(LIB)

clean:
	rm -f *.o $(TARGET) 
