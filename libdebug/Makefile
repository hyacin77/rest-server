CC	= g++
CFLAGS = -std=c++11 -g -O2 -Wno-write-strings -fPIC \
		  -I../include \
		  -D_REENTRANT


LDFLAGS	= -L./ -lpthread \


LIBNAME = libdebug.so

TARGET	= test

OBJS	= Debug.o \

all	: $(TARGET)

clean	:
	rm -f *.o $(OBJS) $(TARGET) $(LIBNAME)
	rm -f ../$(LIBNAME) 

$(LIBNAME)	: $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)
	cp $(LIBNAME) ../

$(TARGET)    : test.cpp $(LIBNAME)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS) -ldebug

.cpp.o	:
	$(CC) $(CFLAGS) -o $@ -c $<
