
DESTDIR=/usr/local
rcDESTDIR=/
SHELL=/bin/sh
VERSION=1.10.10
DEBSUBV=001
INCLUDE=-Iqiconn/include

# for linux
DEBUG=-g
CPPFLAGS=${DEBUG} -O2
INCLUDE=-Iqiconn/include 
MYSQLCONFIG=mysql_config

# for macosX
#DEBUG=-g -stdlib=libstdc++
#CPPFLAGS=-stdlib=libstdc++ -Wnon-virtual-dtor ${DEBUG}
#LDFLAGS=-L/opt/local/lib
#INCLUDE=-I/opt/local/include -Iqiconn/include
#MYSQLCONFIG=/opt/local/lib/mysql55/bin/mysql_config

USEMYSQL=-DUSEMYSQL

# be careful with one : it is destroyed at cleaning !!!
# it is needed only to build debian packages
workplace=/tmp/qigong-build-$$USER


# mysql lib dev package : libmysqlclient-dev
# libmemcached (with a D !) dev package : libmemcached-dev
# librrd dev package : librrd-dev

default:
	@echo "interesting targets : all , install , install_qigong ..."

tarfiles.txt:
	( cd qiconn && repozclean && autoall )
	( git archive master | tar -tvf - | cut -b49- | grep -v debian-src ; find qiconn | grep -v '\.git' ) > tarfiles.txt

tar: tarfiles.txt qicollect.cpp qigong.cpp qiconn/include/qiconn/qiconn.h qiconn/qiconn.cpp
	tar -zcphf qigong_"${VERSION}".orig.tar.gz --no-recursion --transform='s=^='qigong_"${VERSION}"'/=' -T tarfiles.txt
	tar -ztvf qigong_"${VERSION}".orig.tar.gz

all: qigong qicollect qigong.rc qicollect.rc qigong-nomc qigenkey crtelnet
otherall: qigong qigong.rc qicollect.rc qigong-nomc qigenkey crtelnet

allstrip: all
	strip qigong qicollect qigong-nomc

install: allstrip
	chmod 700 ./installscript
	./installscript "${DESTDIR}"
	# update-rc.d qigong start 20 2 3 4 5 . stop 20 0 1 6 .

install_qigong: qigong qigong.rc qigenkey crtelnet
	strip qigong
	strip qigenkey
	strip crtelnet
	chmod 700 ./installscript
	./installscript "${DESTDIR}" qigong "${rcDESTDIR}"

install_qigong-nomc: qigong-nomc qigong.rc qigenkey crtelnet
	strip qigong-nomc
	strip qigenkey
	strip crtelnet
	cp -a qigong-nomc qigong
	./installscript "${DESTDIR}" qigong "${rcDESTDIR}"
	rm qigong

install_qicollect: qicollect qicollect.rc
	strip qicollect
	chmod 700 ./installscript
	./installscript "${DESTDIR}" qicollect "${rcDESTDIR}"


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

# testdirproc: libqiconn.a testdirproc.o
#	g++  ${DEBUG} -Wall -o testdirproc testdirproc.o libqiconn.a

watchconn.o: watchconn.cpp
	g++  ${DEBUG} ${INCLUDE} -Wall -c watchconn.cpp

watchconn: qiconn/libqiconn.a watchconn.o
	g++  ${DEBUG} ${INCLUDE} -Wall -o watchconn qiconn/libqiconn.a watchconn.o

WATCHCONNvimtest: watchconn all
	# ./testdirproc :::80 ::ffff:c700:0001:80 127.0.0.1:80 192.168.132.182:80 127.0.0.1:3306 | tr ':' ';'
	# ./testdirproc :::80 ::ffff:c700:0001:80 127.0.0.1:80 192.168.132.182:80 127.0.0.1:3306  0.0.0.0:1264
	./watchconn -help


vimtest: all localhost.key.priv
	./qicollect -port 1266 -nofork -logfile=logs -conffile=/home/jd/qic-goji/qicollect.conf -rrdpath=/tmp/rrd -keydir=/home/jd/qic-goji/keys
	####	# ./crtelnet 127.0.0.1:1364 localhost.key.priv
	####	# exit 1
	####	### ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &
	####	killall qigong || true
	####	 # ./qigong           -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364
	####	 ./qigong   -debugcrypt -nofork -localkey=`pwd`/localhost.key.priv       -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 2>&1   &
	####	rm -f *.rrd
	####	./qicollect -debugcrypt -keydir=. -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 
	####	# ./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 && ( telnet localhost 1364 ; tail testqigong.log ) 

testqigong: qigong
	./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log

qigong.rc: qigong.rc.proto
	sed "s=@@PREFIX@@=${DESTDIR}=g" < qigong.rc.proto > qigong.rc

qicollect.rc: qicollect.rc.proto
	sed "s=@@PREFIX@@=${DESTDIR}=g" < qicollect.rc.proto > qicollect.rc

qicollect: qicollect.o qicrypt.o qiconn/libqiconn.a qimeasure.o
	g++ ${CPPFLAGS} ${INCLUDE} ${LDFLAGS} `${MYSQLCONFIG} --cflags` -Wall -o qicollect  qicollect.o qicrypt.o qiconn/libqiconn.a qimeasure.o -L /usr/local/lib -lrrd -lmemcached `${MYSQLCONFIG} --libs` -lmcrypt -lmhash

qigong: qigong.o qiconn/libqiconn.a qimeasure.o qicrypt.o
	g++ ${CPPFLAGS} ${INCLUDE} ${LDFLAGS} `${MYSQLCONFIG} --cflags` -Wall -o qigong qigong.o qiconn/libqiconn.a qicrypt.o qimeasure.o -lmemcached `${MYSQLCONFIG} --libs` -lmcrypt -lmhash


qigong-nomc: qigong.o qiconn/libqiconn.a qimeasure-nomc.o qicrypt.o
	g++ ${CPPFLAGS} ${INCLUDE} ${LDFLAGS} `${MYSQLCONFIG} --cflags` -Wall -o qigong-nomc qigong.o qiconn/libqiconn.a qicrypt.o qimeasure-nomc.o -lmcrypt -lmhash



qigong.o: qigong.cpp qiconn/libqiconn.a qigong.h qimeasure.h qicrypt.h
	g++ ${CPPFLAGS} ${INCLUDE} -DQIVERSION="\"${VERSION}\"" `${MYSQLCONFIG} --cflags` -Wall -c qigong.cpp

qicollect.o: qicollect.cpp qiconn/include/qiconn/qiconn.h qicollect.h qimeasure.h qicrypt.h
	g++ ${CPPFLAGS} ${INCLUDE} -DQIVERSION="\"${VERSION}\"" `${MYSQLCONFIG} --cflags` -Wall -c qicollect.cpp

qiconn/configure: qiconn/configure.ac
	( cd qiconn ; autoall )

qiconn/Makefile: qiconn/configure
	( cd qiconn ; ./configure )

qiconn/libqiconn.a: qiconn/qiconn.cpp qiconn/include/qiconn/qiconn.h qiconn/Makefile
	( export MAKEDEBUG="${DEBUG}" ; cd qiconn ; make libqiconn.a )

qimeasure-nomc.o: qimeasure.cpp qimeasure.h
	g++ ${CPPFLAGS} ${INCLUDE} `${MYSQLCONFIG} --cflags` -Wall -c qimeasure.cpp -o qimeasure-nomc.o

qimeasure.o: qimeasure.cpp qimeasure.h
	g++ ${CPPFLAGS} ${INCLUDE} -DUSEMEMCACHED -DUSEMYSQL `${MYSQLCONFIG} --cflags` -Wall -c qimeasure.cpp


qicrypt.o: qicrypt.cpp qicrypt.h qiconn/include/qiconn/qiconn.h
	g++ ${CPPFLAGS} ${INCLUDE} -Wall -c qicrypt.cpp

qigenkey.o: qigenkey.cpp qicrypt.h
	g++ ${CPPFLAGS} ${INCLUDE} -Wall -c qigenkey.cpp

qigenkey: qigenkey.o qicrypt.o qiconn/libqiconn.a
	g++ ${CPPFLAGS} ${INCLUDE} ${LDFLAGS} -Wall -o qigenkey qigenkey.o  qicrypt.o qiconn/libqiconn.a -lmcrypt -lmhash

crtelnet.o: crtelnet.cpp qicrypt.h
	g++ ${CPPFLAGS} ${INCLUDE} -Wall -c crtelnet.cpp

crtelnet: crtelnet.o qicrypt.o qiconn/libqiconn.a
	g++ ${CPPFLAGS} ${INCLUDE} ${LDFLAGS} -Wall -o crtelnet crtelnet.o  qicrypt.o qiconn/libqiconn.a -lmcrypt -lmhash


qigong.h: qicommon.h

qicollect.h: qicommon.h

qimeasure.h: qicommon.h

localhost.key.priv: qigenkey
	rm -f localhost.key.priv
	./qigenkey localhost

clean:
	cd qiconn ; test -f Makefile && make clean || exit 0
	rm -f qigong.o qigong qicollect.o qicollect qimeasure.o watchconn.o watchconn qimeasure-nomc.o qigong-nomc
	rm -f qigenkey.o crtelnet.o qicrypt.o qigenkey crtelnet
	rm -f qigong.rc qicollect.rc
	rm -f testqigong.log testqicoll.log
	rm -f *_testlastfile.rrd *_testfiles.rrd  *_testglobal.rrd  *_testnet.rrd *_testmem.rrd *_testload.rrd *_testfree.rrd
	rm -f localhost.key.priv
	rm -f qigong-*.tgz
	rm -f qigong.dox
	rm -f hex2base64 hex2base64.o
	rm -rf qigong-doc/*
	rm -f tarfiles.txt

distclean: clean debian-clean
	cd qiconn ; test -f Makefile && make distclean || exit 0

qigong.dox: qigong.dox.proto
	sed 's/@@VERSION@@/${VERSION}-${DEBSUBV}/' < qigong.dox.proto > qigong.dox

doc: *.h *.cpp qigong.dox
	doxygen qigong.dox

.PHONY: clean

.PHONY: distclean


debian-qigong-nomc: 
	make clean
	make DESTDIR=/usr qigong.rc
	rm -rf ${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`
	make debpackage=qigong-nomc systarget=`lsb_release -c -s` DESTDIR=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcDESTDIR=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install_qigong-nomc
	make debpackage=qigong-nomc systarget=`lsb_release -c -s` DESTDIR=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcDESTDIR=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install-deb-qigong-nomc

install-deb-qigong-nomc:
	echo ${DESTDIR}
	test -d ${DESTDIR}/../DEBIAN-qigong-nomc || mkdir -p -m755 ${DESTDIR}/../DEBIAN
	for NOM in \
		DEBIAN-qigong-nomc/control	    \
		DEBIAN-qigong-nomc/conffiles	    \
		DEBIAN-qigong-nomc/postinst	    \
		DEBIAN-qigong-nomc/prerm	    \
		DEBIAN-qigong-nomc/postrm	    \
	; do \
	    if [ -f "$$NOM".${systarget} ] ; then   \
		SOURCE="$$NOM".${systarget} ;	    \
	    else				    \
		SOURCE="$$NOM" ;		    \
	    fi ;				    \
	    SHORTNAME=`echo "$$NOM" | rev | cut -d/ -f1 | rev` ; \
	    cp -a "$$SOURCE" ${DESTDIR}/../DEBIAN/"$$SHORTNAME" ;  \
	    sed 's/@@VERSION@@/${VERSION}-${DEBSUBV}/' < "$$SOURCE" > ${DESTDIR}/../DEBIAN/"$$SHORTNAME" ;  \
	done
	mkdir -p ${rcDESTDIR}/etc/qigong/keys
	find ${DESTDIR}/share/man -type f -regex '.*\.[0-9]' -exec gzip -f -9 '{}' \;
	mkdir -m755 -p ${DESTDIR}/share/doc/${debpackage}
	gzip -f -9 -c ChangeLog > ${DESTDIR}/share/doc/${debpackage}/changelog.gz
	gzip -f -9 -c DEBIAN-qigong-nomc/changelog.Debian > ${DESTDIR}/share/doc/${debpackage}/changelog.Debian.gz
	cp DEBIAN-all/copyright ${DESTDIR}/share/doc/${debpackage}
	(export DEBNAME=`echo ${DESTDIR} | rev | cut -d/ -f2 | rev` ; cd ${DESTDIR}/../.. ; ls -ld $$DEBNAME ; fakeroot dpkg-deb --build $$DEBNAME ; lintian $$DEBNAME.deb)
	
	

debian-qigong-full: 
	make clean
	make DESTDIR=/usr qigong.rc
	rm -rf ${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`
	make debpackage=qigong-full systarget=`lsb_release -c -s` DESTDIR=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcDESTDIR=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install_qigong
	make debpackage=qigong-full systarget=`lsb_release -c -s` DESTDIR=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcDESTDIR=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install-deb-qigong-full

install-deb-qigong-full:
	echo ${DESTDIR}
	test -d ${DESTDIR}/../DEBIAN-qigong-full || mkdir -p -m755 ${DESTDIR}/../DEBIAN
	for NOM in \
		DEBIAN-qigong-full/control	    \
		DEBIAN-qigong-full/conffiles	    \
		DEBIAN-qigong-full/postinst	    \
		DEBIAN-qigong-full/prerm	    \
		DEBIAN-qigong-full/postrm	    \
	; do \
	    if [ -f "$$NOM".${systarget} ] ; then   \
		SOURCE="$$NOM".${systarget} ;	    \
	    else				    \
		SOURCE="$$NOM" ;		    \
	    fi ;				    \
	    SHORTNAME=`echo "$$NOM" | rev | cut -d/ -f1 | rev` ; \
	    cp -a "$$SOURCE" ${DESTDIR}/../DEBIAN/"$$SHORTNAME" ;  \
	    sed 's/@@VERSION@@/${VERSION}-${DEBSUBV}/' < "$$SOURCE" > ${DESTDIR}/../DEBIAN/"$$SHORTNAME" ;  \
	done
	mkdir -p ${rcDESTDIR}/etc/qigong/keys
	find ${DESTDIR}/share/man -type f -regex '.*\.[0-9]' -exec gzip -f -9 '{}' \;
	mkdir -m755 -p ${DESTDIR}/share/doc/${debpackage}
	gzip -f -9 -c ChangeLog > ${DESTDIR}/share/doc/${debpackage}/changelog.gz
	gzip -f -9 -c DEBIAN-qigong-full/changelog.Debian > ${DESTDIR}/share/doc/${debpackage}/changelog.Debian.gz
	cp DEBIAN-all/copyright ${DESTDIR}/share/doc/${debpackage}
	(export DEBNAME=`echo ${DESTDIR} | rev | cut -d/ -f2 | rev` ; cd ${DESTDIR}/../.. ; ls -ld $$DEBNAME ; fakeroot dpkg-deb --build $$DEBNAME ; lintian $$DEBNAME.deb)
	
.PHONY: debian-qigong
debian-qigong: debian-qigong-full debian-qigong-nomc

debian-qicollect: 
	make clean
	make DESTDIR=/usr qicollect.rc
	rm -rf ${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`
	make debpackage=qicollect systarget=`lsb_release -c -s` DESTDIR=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcDESTDIR=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install_qicollect
	make debpackage=qicollect systarget=`lsb_release -c -s` DESTDIR=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcDESTDIR=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install-deb-qicollect

install-deb-qicollect:
	echo ${DESTDIR}
	test -d ${DESTDIR}/../DEBIAN-qicollect || mkdir -p -m755 ${DESTDIR}/../DEBIAN
	for NOM in \
		DEBIAN-qicollect/control	    \
		DEBIAN-qicollect/conffiles	    \
		DEBIAN-qicollect/postinst	    \
		DEBIAN-qicollect/prerm	    \
		DEBIAN-qicollect/postrm	    \
	; do \
	    if [ -f "$$NOM".${systarget} ] ; then   \
		SOURCE="$$NOM".${systarget} ;	    \
	    else				    \
		SOURCE="$$NOM" ;		    \
	    fi ;				    \
	    SHORTNAME=`echo "$$NOM" | rev | cut -d/ -f1 | rev` ; \
	    cp -a "$$SOURCE" ${DESTDIR}/../DEBIAN/"$$SHORTNAME" ;  \
	    sed 's/@@VERSION@@/${VERSION}-${DEBSUBV}/' < "$$SOURCE" > ${DESTDIR}/../DEBIAN/"$$SHORTNAME" ;  \
	done
	mkdir -p ${rcDESTDIR}/etc/qicollect/keys
	find ${DESTDIR}/share/man -type f -regex '.*\.[0-9]' -exec gzip -f -9 '{}' \;
	mkdir -m755 -p ${DESTDIR}/share/doc/${debpackage}
	gzip -f -9 -c ChangeLog > ${DESTDIR}/share/doc/${debpackage}/changelog.gz
	gzip -f -9 -c DEBIAN-qicollect/changelog.Debian > ${DESTDIR}/share/doc/${debpackage}/changelog.Debian.gz
	cp DEBIAN-all/copyright ${DESTDIR}/share/doc/${debpackage}
	(export DEBNAME=`echo ${DESTDIR} | rev | cut -d/ -f2 | rev` ; cd ${DESTDIR}/../.. ; ls -ld $$DEBNAME ; fakeroot dpkg-deb --build $$DEBNAME ; lintian $$DEBNAME.deb)
	

.PHONY: debian
debian: debian-qigong-full debian-qigong-nomc debian-qicollect
	lintian ${workplace}/*.deb
	ls -l ${workplace}/*.deb

.PHONY: debian-clean
debian-clean:
	rm -rf ${workplace}


hex2base64.o: hex2base64.cpp
	g++ -g -Iqiconn/include  -Wall -c hex2base64.cpp
hex2base64: hex2base64.o qicrypt.o qiconn/libqiconn.a
	g++ ${CPPFLAGS} ${INCLUDE} ${LDFLAGS} -Wall -o hex2base64 hex2base64.o  qicrypt.o qiconn/libqiconn.a -lmcrypt -lmhash

