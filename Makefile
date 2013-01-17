
DEBUG=
#DEBUG=-g
PREFIX=/usr/local
SHELL=/bin/sh
VERSION=1.9.11
INCLUDE=-Iqiconn/include

default:
	@echo "interesting targets : all , install , install_qigong ..."

all: qigong qicollect qigong.rc qicollect.rc qigong-nomc

allstrip: all
	strip qigong qicollect qigong-nomc

install: allstrip
	chmod 700 ./installscript
	./installscript "${PREFIX}"
	# update-rc.d qigong start 20 2 3 4 5 . stop 20 0 1 6 .

install_qigong: qigong qigong.rc
	chmod 700 ./installscript
	./installscript qigong

archive:
	SRCDIR="qigong-${VERSION}" ;					\
	mkdir "qigong-${VERSION}" &&					\
	git archive master | tar -C "qigong-${VERSION}" -xpf - &&	\
	tar -zcpvf "qigong-${VERSION}.tgz" "qigong-${VERSION}"  &&	\
	rm -r "qigong-${VERSION}" &&					\
	echo done : ;							\
	ls -l "qigong-${VERSION}.tgz"

bintar: allstrip
	@( ARCH=`uname -m` ;						\
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
	@( ARCH=`uname -m` ;						\
	   SRCDIR="qigong-${VERSION}-$${ARCH}-debug" ;			\
	   mkdir "$${SRCDIR}" ;						\
	   chmod 700 installscript ;					\
	   cp -a qigong qicollect qigong-nomc qigong.rc qicollect.rc installscript "$${SRCDIR}" ; \
	   cp -a Makefile-binonly "$${SRCDIR}"/Makefile ;		\
	  tar -zcpvf "$${SRCDIR}".tgz "$${SRCDIR}" ;			\
	  ls -l "$${SRCDIR}"/qigong "$${SRCDIR}"/qicollect ;		\
	  rm -r "$${SRCDIR}" ;						\
	  ls -l "$${SRCDIR}".tgz )

#	@( ARCH=`uname -m` ;						\
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
	g++  ${DEBUG} ${INCLUDE} -Wall -c watchconn.cpp

watchconn: qiconn/qiconn.o watchconn.o
	g++  ${DEBUG} ${INCLUDE} -Wall -o watchconn qiconn/qiconn.o watchconn.o

WATCHCONNvimtest: watchconn all
	# ./testdirproc :::80 ::ffff:c700:0001:80 127.0.0.1:80 192.168.132.182:80 127.0.0.1:3306 | tr ':' ';'
	# ./testdirproc :::80 ::ffff:c700:0001:80 127.0.0.1:80 192.168.132.182:80 127.0.0.1:3306  0.0.0.0:1264
	./watchconn -help


vimtest: all
	### ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &
	killall qigong || true
	 ./qigong           -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364
	rm -f *.rrd
	./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 && ( telnet localhost 1364 ; tail testqigong.log )

testqigong: qigong
	./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log

qigong.rc: qigong.rc.proto
	sed "s=@@PREFIX@@=${PREFIX}=g" < qigong.rc.proto > qigong.rc

qicollect.rc: qicollect.rc.proto
	sed "s=@@PREFIX@@=${PREFIX}=g" < qicollect.rc.proto > qicollect.rc

qicollect: qicollect.o qiconn/qiconn.o qimeasure.o
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -o qicollect  -L /usr/local/lib -lrrd -lmemcached  qicollect.o qiconn/qiconn.o qimeasure.o `mysql_config --libs`

qigong: qigong.o qiconn/qiconn.o qimeasure.o
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -o qigong qigong.o qiconn/qiconn.o qimeasure.o -lmemcached `mysql_config --libs`


qigong-nomc: qigong.o qiconn/qiconn.o qimeasure-nomc.o
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -o qigong-nomc qigong.o qiconn/qiconn.o qimeasure-nomc.o



qigong.o: qigong.cpp qiconn/qiconn.o qigong.h qimeasure.h
	g++ ${DEBUG} ${INCLUDE} -DQIVERSION="\"${VERSION}\"" `mysql_config --cflags` -Wall -c qigong.cpp

qicollect.o: qicollect.cpp qiconn/include/qiconn/qiconn.h qicollect.h qimeasure.h
	g++ ${DEBUG} ${INCLUDE} -DQIVERSION="\"${VERSION}\"" `mysql_config --cflags` -Wall -c qicollect.cpp



qiconn/qiconn.o: qiconn/qiconn.cpp qiconn/include/qiconn/qiconn.h
	cd qiconn ; make qiconn.o

qimeasure-nomc.o: qimeasure.cpp qimeasure.h
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -c qimeasure.cpp -o qimeasure-nomc.o

qimeasure.o: qimeasure.cpp qimeasure.h
	g++ ${DEBUG} ${INCLUDE} -DUSEMEMCACHED -DUSEMYSQL `mysql_config --cflags` -Wall -c qimeasure.cpp


qigong.h: qicommon.h

qicollect.h: qicommon.h

qimeasure.h: qicommon.h

clean:
	rm -f qigong.o qigong qicollect.o qicollect qimeasure.o watchconn.o watchconn qimeasure-nomc.o qigong-nomc
	rm -f qigong.rc qicollect.rc
	rm -f testqigong.log testqicoll.log
	rm -f *_testlastfile.rrd *_testfiles.rrd  *_testglobal.rrd  *_testnet.rrd *_testmem.rrd *_testload.rrd *_testfree.rrd
	rm -f qigong-*.tgz
	rm -rf chikung-doc/*
	cd qiconn ; make clean

distclean: clean

doc: *.h *.cpp chikung.dox
	doxygen chikung.dox

.PHONY: clean

.PHONY: distclean
