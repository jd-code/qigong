
#DEBUG=
DEBUG=-g
prefix=/usr/local
rcprefix=/
SHELL=/bin/sh
VERSION=1.10.1
DEBSUBV=000
INCLUDE=-Iqiconn/include

# be careful with one : it is destroyed at cleaning !!!
# it is needed only to build debian packages
workplace=/tmp/chikung-build-$$USER


# mysql lib dev package : libmysqlclient-dev
# libmemcached (with a D !) dev package : libmemcached-dev
# librrd dev package : librrd-dev

default:
	@echo "interesting targets : all , install , install_qigong ..."

all: qigong qicollect qigong.rc qicollect.rc qigong-nomc qigenkey crtelnet

allstrip: all
	strip qigong qicollect qigong-nomc

install: allstrip
	chmod 700 ./installscript
	./installscript "${prefix}"
	# update-rc.d qigong start 20 2 3 4 5 . stop 20 0 1 6 .

install_qigong: qigong qigong.rc
	strip qigong
	chmod 700 ./installscript
	./installscript "${prefix}" qigong "${rcprefix}"

install_qigong-nomc: qigong-nomc qigong.rc
	strip qigong-nomc
	cp -a qigong-nomc qigong
	./installscript "${prefix}" qigong "${rcprefix}"
	rm qigong

install_qicollect: qicollect qicollect.rc
	strip qicollect
	chmod 700 ./installscript
	./installscript "${prefix}" qicollect "${rcprefix}"


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
	# ./crtelnet 127.0.0.1:1290 lkjlkj
	# exit 0
	### ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &
	killall qigong || true
	 # ./qigong           -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364
	 ./qigong   -nofork -localkey=test.key.priv       -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 2>&1   &
	rm -f *.rrd
	./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 
	# ./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 && ( telnet localhost 1364 ; tail testqigong.log ) 

testqigong: qigong
	./qigong ; telnet localhost 1264 ; tail /var/log/qigong.log

qigong.rc: qigong.rc.proto
	sed "s=@@PREFIX@@=${prefix}=g" < qigong.rc.proto > qigong.rc

qicollect.rc: qicollect.rc.proto
	sed "s=@@PREFIX@@=${prefix}=g" < qicollect.rc.proto > qicollect.rc

qicollect: qicollect.o qicrypt.o qiconn/qiconn.o qimeasure.o
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -o qicollect  qicollect.o qicrypt.o qiconn/qiconn.o qimeasure.o -L /usr/local/lib -lrrd -lmemcached `mysql_config --libs` -lmcrypt

qigong: qigong.o qiconn/qiconn.o qimeasure.o qicrypt.o
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -o qigong qigong.o qiconn/qiconn.o qicrypt.o qimeasure.o -lmemcached `mysql_config --libs` -lmcrypt


qigong-nomc: qigong.o qiconn/qiconn.o qimeasure-nomc.o qicrypt.o
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -o qigong-nomc qigong.o qiconn/qiconn.o qicrypt.o qimeasure-nomc.o -lmcrypt



qigong.o: qigong.cpp qiconn/qiconn.o qigong.h qimeasure.h
	g++ ${DEBUG} ${INCLUDE} -DQIVERSION="\"${VERSION}\"" `mysql_config --cflags` -Wall -c qigong.cpp

qicollect.o: qicollect.cpp qiconn/include/qiconn/qiconn.h qicollect.h qimeasure.h
	g++ ${DEBUG} ${INCLUDE} -DQIVERSION="\"${VERSION}\"" `mysql_config --cflags` -Wall -c qicollect.cpp



qiconn/qiconn.o: qiconn/qiconn.cpp qiconn/include/qiconn/qiconn.h
	( export MAKEDEBUG=${DEBUG} ; cd qiconn ; make qiconn.o )

qimeasure-nomc.o: qimeasure.cpp qimeasure.h
	g++ ${DEBUG} ${INCLUDE} `mysql_config --cflags` -Wall -c qimeasure.cpp -o qimeasure-nomc.o

qimeasure.o: qimeasure.cpp qimeasure.h
	g++ ${DEBUG} ${INCLUDE} -DUSEMEMCACHED -DUSEMYSQL `mysql_config --cflags` -Wall -c qimeasure.cpp


qicrypt.o: qicrypt.cpp qicrypt.h qiconn/include/qiconn/qiconn.h
	g++ ${DEBUG} ${INCLUDE} -Wall -c qicrypt.cpp

qigenkey.o: qigenkey.cpp qicrypt.h
	g++ ${DEBUG} ${INCLUDE} -Wall -c qigenkey.cpp

qigenkey: qigenkey.o qicrypt.o qiconn/qiconn.o
	g++ ${DEBUG} ${INCLUDE} -Wall -o qigenkey qigenkey.o  qicrypt.o qiconn/qiconn.o -lmcrypt

crtelnet.o: crtelnet.cpp qicrypt.h
	g++ ${DEBUG} ${INCLUDE} -Wall -c crtelnet.cpp

crtelnet: crtelnet.o qicrypt.o qiconn/qiconn.o
	g++ ${DEBUG} ${INCLUDE} -Wall -o crtelnet crtelnet.o  qicrypt.o qiconn/qiconn.o -lmcrypt


qigong.h: qicommon.h

qicollect.h: qicommon.h

qimeasure.h: qicommon.h

clean:
	rm -f qigong.o qigong qicollect.o qicollect qimeasure.o watchconn.o watchconn qimeasure-nomc.o qigong-nomc
	rm -f qigenkey.o crtelnet.o qicrypt.o qigenkey crtelnet
	rm -f qigong.rc qicollect.rc
	rm -f testqigong.log testqicoll.log
	rm -f *_testlastfile.rrd *_testfiles.rrd  *_testglobal.rrd  *_testnet.rrd *_testmem.rrd *_testload.rrd *_testfree.rrd
	rm -f qigong-*.tgz
	rm -rf chikung-doc/*
	cd qiconn ; make clean

distclean: clean debian-clean

doc: *.h *.cpp chikung.dox
	doxygen chikung.dox

.PHONY: clean

.PHONY: distclean


debian-qigong-nomc: 
	make clean
	make prefix=/usr qigong.rc
	rm -rf ${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`
	make debpackage=qigong-nomc systarget=`lsb_release -c -s` prefix=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcprefix=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install_qigong-nomc
	make debpackage=qigong-nomc systarget=`lsb_release -c -s` prefix=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcprefix=${workplace}/qigong-nomc-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install-deb-qigong-nomc

install-deb-qigong-nomc:
	echo ${prefix}
	test -d ${prefix}/../DEBIAN-qigong-nomc || mkdir -p -m755 ${prefix}/../DEBIAN
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
	    cp -a "$$SOURCE" ${prefix}/../DEBIAN/"$$SHORTNAME" ;  \
	    sed 's/@@VERSION@@/${VERSION}-${DEBSUBV}/' < "$$SOURCE" > ${prefix}/../DEBIAN/"$$SHORTNAME" ;  \
	done
	find ${prefix}/share/man -type f -regex '.*\.[0-9]' -exec gzip -f -9 '{}' \;
	mkdir -m755 -p ${prefix}/share/doc/${debpackage}
	gzip -f -9 -c ChangeLog > ${prefix}/share/doc/${debpackage}/changelog.gz
	gzip -f -9 -c DEBIAN-qigong-nomc/changelog.Debian > ${prefix}/share/doc/${debpackage}/changelog.Debian.gz
	cp DEBIAN-common/copyright ${prefix}/share/doc/${debpackage}
	(export DEBNAME=`echo ${prefix} | rev | cut -d/ -f2 | rev` ; cd ${prefix}/../.. ; ls -ld $$DEBNAME ; fakeroot dpkg-deb --build $$DEBNAME ; lintian $$DEBNAME.deb)
	
	

debian-qigong-full: 
	make clean
	make prefix=/usr qigong.rc
	rm -rf ${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`
	make debpackage=qigong-full systarget=`lsb_release -c -s` prefix=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcprefix=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install_qigong
	make debpackage=qigong-full systarget=`lsb_release -c -s` prefix=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcprefix=${workplace}/qigong-full-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install-deb-qigong-full

install-deb-qigong-full:
	echo ${prefix}
	test -d ${prefix}/../DEBIAN-qigong-full || mkdir -p -m755 ${prefix}/../DEBIAN
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
	    cp -a "$$SOURCE" ${prefix}/../DEBIAN/"$$SHORTNAME" ;  \
	    sed 's/@@VERSION@@/${VERSION}-${DEBSUBV}/' < "$$SOURCE" > ${prefix}/../DEBIAN/"$$SHORTNAME" ;  \
	done
	find ${prefix}/share/man -type f -regex '.*\.[0-9]' -exec gzip -f -9 '{}' \;
	mkdir -m755 -p ${prefix}/share/doc/${debpackage}
	gzip -f -9 -c ChangeLog > ${prefix}/share/doc/${debpackage}/changelog.gz
	gzip -f -9 -c DEBIAN-qigong-full/changelog.Debian > ${prefix}/share/doc/${debpackage}/changelog.Debian.gz
	cp DEBIAN-common/copyright ${prefix}/share/doc/${debpackage}
	(export DEBNAME=`echo ${prefix} | rev | cut -d/ -f2 | rev` ; cd ${prefix}/../.. ; ls -ld $$DEBNAME ; fakeroot dpkg-deb --build $$DEBNAME ; lintian $$DEBNAME.deb)
	
.PHONY: debian-qigong
debian-qigong: debian-qigong-full debian-qigong-nomc

debian-qicollect: 
	make clean
	make prefix=/usr qicollect.rc
	rm -rf ${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`
	make debpackage=qicollect systarget=`lsb_release -c -s` prefix=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcprefix=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install_qicollect
	make debpackage=qicollect systarget=`lsb_release -c -s` prefix=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/usr rcprefix=${workplace}/qicollect-${VERSION}-${DEBSUBV}_"amd64"-`lsb_release -c -s`/ install-deb-qicollect

install-deb-qicollect:
	echo ${prefix}
	test -d ${prefix}/../DEBIAN-qicollect || mkdir -p -m755 ${prefix}/../DEBIAN
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
	    cp -a "$$SOURCE" ${prefix}/../DEBIAN/"$$SHORTNAME" ;  \
	    sed 's/@@VERSION@@/${VERSION}-${DEBSUBV}/' < "$$SOURCE" > ${prefix}/../DEBIAN/"$$SHORTNAME" ;  \
	done
	find ${prefix}/share/man -type f -regex '.*\.[0-9]' -exec gzip -f -9 '{}' \;
	mkdir -m755 -p ${prefix}/share/doc/${debpackage}
	gzip -f -9 -c ChangeLog > ${prefix}/share/doc/${debpackage}/changelog.gz
	gzip -f -9 -c DEBIAN-qicollect/changelog.Debian > ${prefix}/share/doc/${debpackage}/changelog.Debian.gz
	cp DEBIAN-common/copyright ${prefix}/share/doc/${debpackage}
	(export DEBNAME=`echo ${prefix} | rev | cut -d/ -f2 | rev` ; cd ${prefix}/../.. ; ls -ld $$DEBNAME ; fakeroot dpkg-deb --build $$DEBNAME ; lintian $$DEBNAME.deb)
	

.PHONY: debian
debian: debian-qigong-full debian-qigong-nomc debian-qicollect
	lintian ${workplace}/*.deb
	ls -l ${workplace}/*.deb

.PHONY: debian-clean
debian-clean:
	rm -rf ${workplace}

