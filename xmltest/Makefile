#CC=arm-buildroot-linux-gnueabihf-g++
CC=g++
CFLAGS= -g -O0 
TARGET = test
LDFLAGS=
IFLAGS=
LIBS=-ltinyxml2

$(TARGET): main.o 
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^  $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^  $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o $(TARGET) 
