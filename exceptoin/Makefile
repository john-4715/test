CC=arm-buildroot-linux-gnueabihf-g++
#CC=g++
CFLAGS= -O0 
TARGET = test
LDFLAGS=-L./lib  -L./libjson -rdynamic -funwind-tables -ffunction-sections -finstrument-functions -no-pie -lm -ldl -lrt -lTest -lJsonObjects
IFLAGS=-I./lib -I./libjson

$(TARGET): main.o 
	$(CC)  $(IFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^  -lTest 

%.o: %.cpp
	$(CC) $(IFLAGS) $(CFLAGS) $(LDFLAGS) -c -o $@ $^ 

clean:
	rm -f app *.o $(TARGET) 
