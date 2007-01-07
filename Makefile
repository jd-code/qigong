
#DEBUG=-g
DEBUG=
PREFIX=/usr/local
SHELL=/bin/sh
VERSION="1.0.1"

default:
	@echo "interesting targets : all , install , install_qigong ..."

all: qigong qicollect qigong.rc qicollect.rc

allstrip: all
	strip qigong qicollect

install: allstrip
	./installscript "${PREFIX}"

install_qigong: qigong qigong.rc
	./installscript qigong

bintar: allstrip
	@( ARCH=`arch` ;						\
	   SRCDIR="qigong-${VERSION}-$${ARCH}" ;			\
	   mkdir "$${SRCDIR}" ;						\
	   cp -a qigong qicollect qigong.rc qicollect.rc Makefile installscript "$${SRCDIR}" ; \
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

vimtest: all
	# ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &
	./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364
	./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugccstates && ( telnet localhost 1364 ; tail testqigong.log )

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
	rm -f qiconn.o qigong.o qigong qicollect.o qicollect qimeasure.o
	rm -f qigong.rc qicollect.rc prefix.sed
	rm -f testqigong.log testqicoll.log
	rm -f *_testfiles.rrd  *_testglobal.rrd  *_testnet.rrd *_testmem.rrd *_testload.rrd

distclean: clean

.PHONY: clean

.PHONY: distclean
