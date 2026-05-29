CC=arm-buildroot-linux-gnueabihf-gcc
#CC=gcc
CFLAGS= -O0 
LDFLAGS=-rdynamic -funwind-tables -ffunction-sections -finstrument-functions -no-pie -Wl,-rpath=../lib -L../lib -ldl
IFLAGS=-I../lib
app_test: main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(IFLAGS) -lTest

%.o: %.c
	$(CC) $(IFLAGS) $(CFLAGS) $(LDFLAGS) -c -o $@ $^ 

clean:
	rm -f app_test *.o $(TARGET) 
