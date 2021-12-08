CC	= g++
CFLAGS = -std=c++11 -g -O2 -Wno-write-strings -fPIC \
		  -I../include \
		  -I../boost_1_71_0/ \
		  -D_REENTRANT


LDFLAGS	= -L../ \
	  -lcrypt \
	  -lpthread \
	  -lrt \
	  -lssl \
	  -lcrypto \
	  -lz \
          -ldebug \


TARGET	= libhttps.so 

OBJS	= HttpsServer.o \
	  HttpsClient.o \

all	: $(TARGET)

clean	:
	rm -f *.o $(OBJS) $(TARGET)
	rm -f ../$(TARGET)

libhttps.so	: $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)
	cp $(TARGET) ../

.cpp.o	:
	$(CC) $(CFLAGS) -o $@ -c $<
