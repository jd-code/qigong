
#DEBUG=-g
DEBUG=

all: qigong qicollect

vimtest: all
	# ./qicollect
	# ./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log
	./qigong -debugout ; ./qicollect -nofork && ( telnet localhost 1264 ; tail /var/log/qigong.log )

testqigong: qigong
	./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log


qicollect: qicollect.o qiconn.o qimeasure.o
	g++ ${DEBUG} -Wall -o qicollect  -L /usr/local/lib -lrrd   qicollect.o qiconn.o qimeasure.o

qigong: qigong.o qiconn.o qimeasure.o
	g++ ${DEBUG} -Wall -o qigong qigong.o qiconn.o qimeasure.o



qigong.o: qigong.cpp qiconn.h qigong.h qimeasure.h
	g++ ${DEBUG} -Wall -c qigong.cpp

qicollect.o: qicollect.cpp qiconn.h qicollect.h qimeasure.h
	g++ ${DEBUG} -Wall -c qicollect.cpp



qiconn.o: qiconn.cpp qiconn.h
	g++ ${DEBUG} -Wall -c qiconn.cpp

qimeasure.o: qimeasure.cpp qimeasure.h
	g++ ${DEBUG} -Wall -c qimeasure.cpp



clean:
	rm -f qiconn.o qigong.o qigong qicollect.o qicollect

distclean: clean

.PHONY: clean

.PHONY: distclean
