#CC=arm-buildroot-linux-gnueabihf-gcc
CC=gcc
CFLAGS= -O0 
TARGET = test
LDFLAGS=-rdynamic -funwind-tables -ffunction-sections -finstrument-functions -no-pie -lm -ldl -lrt 
IFLAGS=

$(TARGET): main.o 
	$(CC)  $(IFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ 

%.o: %.cpp
	$(CC) $(IFLAGS) $(CFLAGS) $(LDFLAGS) -c -o $@ $^ 

clean:
	rm -f app *.o $(TARGET) 
