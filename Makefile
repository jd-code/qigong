
#DEBUG=-g
DEBUG=
PREFIX=/usr/local
SHELL=/bin/sh

all: qigong qicollect qigong.rc

install: qigong qicollect
	./installscript "${PREFIX}"

vimtest: all
	# ./qicollect
	# ./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log
	# ./qigong -debugout ; ./qicollect ; tail /var/log/qigong.log
	./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout
	./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -nofork && ( telnet localhost 1264 ; tail /var/log/qigong.log )

testqigong: qigong
	./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log


qigong.rc: qigong.rc.proto
	( echo ': reboucle' ; echo 's/\//_slash_/' ; echo 't reboucle' ; \
	  echo ': boucle2' ; echo 's/_slash_/\\\//' ; echo 't boucle2' ) > /tmp/build_qigong.sed
	( PREFIXSUB=`echo "${PREFIX}" | sed -f /tmp/build_qigong.sed` ; sed "s/@@PREFIX@@/$${PREFIXSUB}/" < qigong.rc.proto > qigong.rc )

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
	rm -f qiconn.o qigong.o qigong qicollect.o qicollect qimeasure.o qigong.rc testqigong.log testqicoll.log

distclean: clean

.PHONY: clean

.PHONY: distclean
