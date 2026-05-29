#CC=gcc
CC=arm-buildroot-linux-gnueabihf-gcc
CFLAGS=-Wall -g -O2
TARGET=sftp_client

#LDFLAGS=-L/usr/local/lib
#IFLAGS=-I/usr/local/include

LDFLAGS=-L/opt/arm32/libssh2/lib
IFLAGS=-I//opt/arm32/libssh2/include

LIB=-lpthread -lssh2

OBJS = sftp_client.o 
$(TARGET): $(OBJS)
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB)

%.o: %.c
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^ $(LDFLAGS) $(LIB)

clean:
	rm -f app *.o $(TARGET) 
