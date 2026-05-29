#CC=arm-buildroot-linux-gnueabihf-g++
CC=g++
CFLAGS=-Wall -std=c++11 -g -fsanitize=address -fno-omit-frame-pointer
TARGET=email_validator

LDFLAGS=
IFLAGS=
LIB=

OBJS = email_validator.o 
$(TARGET): $(OBJS)
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB)

%.o: %.cpp
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^ $(LDFLAGS) $(LIB)

clean:
	rm -f *.o $(TARGET) 
