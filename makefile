PREFIX=~/.local

CFLAGS=-I./include/ -Wno-incompatible-pointer-types -Wno-format-extra-args -Wno-deprecated-declarations -fno-stack-protector -Wall -pedantic -g -Wpedantic -Ofast

CSRCS=$(shell find ./src/ -type f -name "*.c")
OBJS=$(CSRCS:.c=.o)

linux: CC=gcc
linux: PKG_CONFIG=pkg-config
linux: OS_CFLAGS=`$(PKG_CONFIG) --cflags gtk+-3.0` `$(PKG_CONFIG) --libs gtk+-3.0` -lX11 -lXtst -lpthread -lXfixes

windows: CC=x86_64-w64-mingw32.static-gcc
windows: PKG_CONFIG=x86_64-w64-mingw32.static-pkg-config
windows: OS_CFLAGS=`$(PKG_CONFIG) --cflags gtk+-3.0` `$(PKG_CONFIG) --libs gtk+-3.0` -lws2_32 

all: linux

linux: ${OBJS}
	$(CC) -o ./mindcontrol ${OBJS} ${CFLAGS} ${OS_CFLAGS}

windows: ${OBJS}
	$(CC) -o ./mindcontrol ${OBJS} ${CFLAGS} ${OS_CFLAGS}

%.o: %.c
	$(CC) -c ${CFLAGS} ${OS_CFLAGS} -o $@ $<

clean:
	find ./ -type f -name '*.o' -delete

install:
	cp ./mindcontrol ${PREFIX}/bin/

tc: all
	tmf -s -i 192.168.1.41 -p 4321 ./mindcontrol
	sudo ./mindcontrol t
