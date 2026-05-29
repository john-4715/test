#CC=arm-buildroot-linux-gnueabihf-g++
CC=g++
CFLAGS=-Wall -std=c++11 -g -fsanitize=address -fno-omit-frame-pointer
TARGET=getCertExpire

LDFLAGS=-L./libjson  
IFLAGS=-I./libjson
LIB=-levent -levent_openssl -lssl -lcrypto 

OBJS = main.o 
$(TARGET): $(OBJS)
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB)

%.o: %.c
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^ $(LDFLAGS) $(LIB)

clean:
	rm -f app *.o $(TARGET) 
