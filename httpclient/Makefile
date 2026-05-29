#CC=arm-buildroot-linux-gnueabihf-g++
CC=g++
CFLAGS= -g -O0 
TARGET = httpclient
LDFLAGS=
IFLAGS=
LIBS=-levent

$(TARGET): main.o http_client.o
	$(CC)  $(IFLAGS) $(CFLAGS) -o $@ $^  $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CC) $(IFLAGS) $(CFLAGS) -c -o $@ $^  $(LDFLAGS) $(LIBS)

clean:
	rm -f app *.o $(TARGET) 
