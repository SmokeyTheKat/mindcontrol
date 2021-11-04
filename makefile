PREFIX=~/.local/bin

CSRCS=$(shell find ./src/ -type f -name "*.c")

all:
	gcc ${CSRCS} -o ./mindcontrol -I./include/ -Wall -g -lX11 -lXtst -lpthread
tc: all
	tmf -s -i 192.168.1.41 -p 1234 ./mindcontrol
	sudo ./mindcontrol c