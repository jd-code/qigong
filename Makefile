
DEBUG=
#DEBUG=-g
PREFIX=/usr/local
SHELL=/bin/sh
VERSION="1.7.1"

default:
	@echo "interesting targets : all , install , install_qigong ..."

all: qigong qicollect qigong.rc qicollect.rc

allstrip: all
	strip qigong qicollect

install: allstrip
	chmod 700 ./installscript
	./installscript "${PREFIX}"
	# update-rc.d qigong start 20 2 3 4 5 . stop 20 0 1 6 .

install_qigong: qigong qigong.rc
	chmod 700 ./installscript
	./installscript qigong

bintar: allstrip
	@( ARCH=`arch` ;						\
	   SRCDIR="qigong-${VERSION}-$${ARCH}" ;			\
	   mkdir "$${SRCDIR}" ;						\
	   chmod 700 installscript ;					\
	   cp -a qigong qicollect qigong.rc qicollect.rc installscript "$${SRCDIR}" ; \
	   cp -a Makefile-binonly "$${SRCDIR}"/Makefile ;		\
	  tar -zcpvf "$${SRCDIR}".tgz "$${SRCDIR}" ;			\
	  ls -l "$${SRCDIR}"/qigong "$${SRCDIR}"/qicollect ;		\
	  rm -r "$${SRCDIR}" ;						\
	  ls -l "$${SRCDIR}".tgz )

bintardebug: all
	@( ARCH=`arch` ;						\
	   SRCDIR="qigong-${VERSION}-$${ARCH}-debug" ;			\
	   mkdir "$${SRCDIR}" ;						\
	   chmod 700 installscript ;					\
	   cp -a qigong qicollect qigong.rc qicollect.rc installscript "$${SRCDIR}" ; \
	   cp -a Makefile-binonly "$${SRCDIR}"/Makefile ;		\
	  tar -zcpvf "$${SRCDIR}".tgz "$${SRCDIR}" ;			\
	  ls -l "$${SRCDIR}"/qigong "$${SRCDIR}"/qicollect ;		\
	  rm -r "$${SRCDIR}" ;						\
	  ls -l "$${SRCDIR}".tgz )

#	@( ARCH=`arch` ;						\
#	  SRCDIR="qigong-${VERSION}-$${ARCH}" ;				\
#	  svn export . "$${SRCDIR}" ;					\
#	  ( cd "$${SRCDIR}" ; make allstrip ) ;				\
#	  tar -zcpvf "$${SRCDIR}".tgz "$${SRCDIR}" ;			\
#	  ls -l "$${SRCDIR}"/qigong "$${SRCDIR}"/qicollect ;		\
#	  rm -r "$${SRCDIR}" ;						\
#	  ls -l "$${SRCDIR}".tgz )

# testdirproc.o: testdirproc.cpp
#	g++  ${DEBUG} -Wall -c testdirproc.cpp

# testdirproc: qiconn.o testdirproc.o
#	g++  ${DEBUG} -Wall -o testdirproc testdirproc.o qiconn.o

watchconn.o: watchconn.cpp
	g++  ${DEBUG} -Wall -c watchconn.cpp

watchconn: qiconn.o watchconn.o
	g++  ${DEBUG} -Wall -o watchconn qiconn.o watchconn.o

vimtest: watchconn all
	# ./testdirproc :::80 ::ffff:c700:0001:80 127.0.0.1:80 192.168.132.182:80 127.0.0.1:3306 | tr ':' ';'
	# ./testdirproc :::80 ::ffff:c700:0001:80 127.0.0.1:80 192.168.132.182:80 127.0.0.1:3306  0.0.0.0:1264
	./watchconn -help


oldvimtest: all
	# ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &
	./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364
	./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 && ( telnet localhost 1364 ; tail testqigong.log )

testqigong: qigong
	./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log

prefix.sed:
	@echo creating prefix.sed
	@( echo ': reboucle' ; echo 's/\//_slash_/' ; echo 't reboucle' ; \
	  echo ': boucle2' ; echo 's/_slash_/\\\//' ; echo 't boucle2' ) > prefix.sed

qigong.rc: qigong.rc.proto prefix.sed
	( PREFIXSUB=`echo "${PREFIX}" | sed -f prefix.sed` ; sed "s/@@PREFIX@@/$${PREFIXSUB}/" < qigong.rc.proto > qigong.rc )

qicollect.rc: qicollect.rc.proto
	( PREFIXSUB=`echo "${PREFIX}" | sed -f prefix.sed` ; sed "s/@@PREFIX@@/$${PREFIXSUB}/" < qicollect.rc.proto > qicollect.rc )

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
	rm -f qiconn.o qigong.o qigong qicollect.o qicollect qimeasure.o watchconn.o watchconn
	rm -f qigong.rc qicollect.rc prefix.sed
	rm -f testqigong.log testqicoll.log
	rm -f *_testlastfile.rrd *_testfiles.rrd  *_testglobal.rrd  *_testnet.rrd *_testmem.rrd *_testload.rrd *_testfree.rrd
	rm -f qigong-*.tgz

distclean: clean

.PHONY: clean

.PHONY: distclean
