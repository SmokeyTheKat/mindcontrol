PREFIX=~/.local/bin

CSRCS=$(shell find ./src/ -type f -name "*.c")

all: linux windows
linux:
	gcc -o ./mindcontrol ${CSRCS} -I./include/ -Ofast -Wall -g -lX11 -lXtst -lpthread
windows:
	x86_64-w64-mingw32-gcc -o ./w_mindcontrol $(CSRCS) -Ofast -I./include/ -lws2_32
tc: all
	tmf -s -i 192.168.1.41 -p 1234 ./mindcontrol
	sudo ./mindcontrol c