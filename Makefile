
all: qigong qicollect

vimtest: all
	# ./qicollect
	# ./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log
	./qigong ; ./qicollect ; telnet localhost 1264 ; tail /var/log/qigong.log

qicollect: qicollect.o qiconn.o
	g++ -Wall -o qicollect  -L /usr/local/lib -lrrd   qicollect.o qiconn.o

qigong: qigong.o qiconn.o
	g++ -Wall -o qigong qigong.o qiconn.o

qigong.o: qigong.cpp qiconn.h qigong.h
	g++ -Wall -c qigong.cpp

qicollect.o: qicollect.cpp qiconn.h qicollect.h
	g++ -Wall -c qicollect.cpp

qiconn.o: qiconn.cpp qiconn.h
	g++ -Wall -c qiconn.cpp

clean:
	rm -f qiconn.o qigong.o qigong qicollect.o qicollect

distclean: clean

.PHONY: clean

.PHONY: distclean
