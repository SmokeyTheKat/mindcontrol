PREFIX=~/.local

CFLAGS=-I./include/ -Ofast -fno-stack-protector -Wall -g
CSRCS=$(shell find ./src/ -type f -name "*.c")

LINUX_CC=gcc
LINUX_PKG_CONFIG=pkg-config
LINUX_CFLAGS=`$(LINUX_PKG_CONFIG) --cflags gtk+-3.0` `$(LINUX_PKG_CONFIG) --libs gtk+-3.0` -lX11 -lXtst -lpthread

WIN_CC=x86_64-w64-mingw32.static-gcc
WIN_PKG_CONFIG=x86_64-w64-mingw32.static-pkg-config
WIN_CFLAGS=-lws2_32 `$(WIN_PKG_CONFIG) --cflags gtk+-3.0` `$(WIN_PKG_CONFIG) --libs gtk+-3.0`

all: linux windows
linux:
	$(LINUX_CC) -o ./mindcontrol ${CSRCS} ${CFLAGS} ${LINUX_CFLAGS}
windows:
	$(WIN_CC) -o ./mindcontrol.exe $(CSRCS) ${CFLAGS} ${WIN_CFLAGS} 
gui:
	$(LINUX_CC) -o ./mindcontrol ./src/gui/gui.c `$(LINUX_PKG_CONFIG) --cflags --libs gtk+-3.0`
	./mindcontrol
gui_windows:
	$(WIN_CC) -o ./mindcontrol.exe ./src/gui/gui.c `$(WIN_PKG_CONFIG) --cflags --libs gtk+-3.0`
	./mindcontrol
install:
	cp ./mindcontrol ${PREFIX}/bin/
tc: all
	tmf -s -i 192.168.1.41 -p 1234 ./mindcontrol
	sudo ./mindcontrol -port 1269 c