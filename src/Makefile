#can't currently compile embedded target in here, still need to copy relevant files to ARM/Keil MDK project folder
CFLAGS = -W -Wall -g
CC = gcc

DIR_BIN = ../bin

all: master_test scale_test master_webserver

.PHONY : master_test
master_test: ${DIR_BIN}/master_test
.PHONY : scale_test
scale_test: ${DIR_BIN}/scale_test

${DIR_BIN}/master_test: master_test.o zb_packets_api.o zb_transport_tty.o requesthandlers.o
	gcc -o ${DIR_BIN}/master_test -lpthread master_test.o zb_packets_api.o zb_transport_tty.o requesthandlers.o

${DIR_BIN}/scale_test: scale_test.o zb_packets_api.o zb_transport_tty.o
	gcc -o ${DIR_BIN}/scale_test -lpthread scale_test.o zb_packets_api.o zb_transport_tty.o

clean:
	rm -f *.o
	cd ${DIR_BIN}
	rm -f master_test scale_test master_webserver *.exe

