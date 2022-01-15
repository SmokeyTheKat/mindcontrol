PREFIX=~/.local

CFLAGS=-I./include/ -Wno-incompatible-pointer-types -Wno-format-extra-args -Wno-deprecated-declarations -fno-stack-protector -Wall -pedantic -g -Wpedantic -Ofast

CSRCS=$(shell find ./src/ -type f -name "*.c")

LINUX_CC=gcc
LINUX_PKG_CONFIG=pkg-config
LINUX_CFLAGS=`$(LINUX_PKG_CONFIG) --cflags gtk+-3.0` `$(LINUX_PKG_CONFIG) --libs gtk+-3.0` -lX11 -lXtst -lpthread -lXfixes

#WIN_CC=x86_64-w64-mingw32-gcc
#WIN_PKG_CONFIG=x86_64-w64-mingw32-pkg-config
#WIN_CFLAGS=`$(WIN_PKG_CONFIG) --static --cflags gtk+-3.0` `$(WIN_PKG_CONFIG) --static --libs gtk+-3.0` -lws2_32 -lpthread

WIN_CC=x86_64-w64-mingw32.static-gcc
WIN_PKG_CONFIG=x86_64-w64-mingw32.static-pkg-config
WIN_CFLAGS=`$(WIN_PKG_CONFIG) --cflags gtk+-3.0` `$(WIN_PKG_CONFIG) --libs gtk+-3.0` -lws2_32 


all: linux
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
	tmf -s -i 192.168.1.41 -p 4321 ./mindcontrol
	sudo ./mindcontrol t
