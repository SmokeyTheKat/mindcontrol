PREFIX=~/.local

CFLAGS=-Ofast -I./include/ -Wno-incompatible-pointer-types -Wno-format-extra-args -Wno-deprecated-declarations -fno-stack-protector -Wall -pedantic -Wpedantic

CSRCS=$(shell find ./src/ -type f -name "*.c")
OBJS=$(CSRCS:.c=.o)

linux: CC=gcc
linux: PKG_CONFIG=pkg-config
linux: OS_CFLAGS=`$(PKG_CONFIG) --cflags gtk+-3.0`
linux: OS_LDFLAGS=`$(PKG_CONFIG) --libs gtk+-3.0` -lX11 -lXtst -lpthread -lXfixes

windows: CC=x86_64-w64-mingw32.static-gcc
windows: PKG_CONFIG=x86_64-w64-mingw32.static-pkg-config
windows: OS_CFLAGS=`$(PKG_CONFIG) --cflags gtk+-3.0`
windows: OS_LDFLAGS=`$(PKG_CONFIG) --libs gtk+-3.0` -lpowrprof -lws2_32 

all: linux

linux: ${OBJS}
	$(CC) -o ./mindcontrol $(OBJS) $(CFLAGS) $(OS_LDFLAGS) $(OS_CFLAGS)

windows: ${OBJS}
	$(CC) -o ./mindcontrol.exe $(OBJS) $(CFLAGS) $(OS_LDFLAGS) $(OS_CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(OS_CFLAGS) -o $@ $<

clean:
	find ./ -type f -name '*.o' -delete

install:
	cp ./mindcontrol $(PREFIX)/bin/

tc: all
	sudo ./mindcontrol t
