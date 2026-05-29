CC=arm-buildroot-linux-gnueabihf-g++
#CC=g++
CFLAGS= -O0 
TARGET = test
LDFLAGS= -L./libjson -rdynamic -funwind-tables -ffunction-sections -finstrument-functions -no-pie -lm -ldl -lrt -lJsonObjects
IFLAGS=-I./libjson

$(TARGET): main.o 
	$(CC)  $(IFLAGS) $(CFLAGS)  -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(IFLAGS) $(CFLAGS)  -c -o $@ $^ $(LDFLAGS)

clean:
	rm -f app *.o $(TARGET) 
