

ACLOCAL_AMFLAGS = -I m4

vimtest: all localhost.key.priv
	./qicollect --bind=127.0.0.1:10080 --user=$$USER --access_log=access_log --earlylog -debugcrypt -nofork -keydir=/home/jd/srcg/conftestqicollect/keys -logfile=testqicoll.log -conffile=/home/jd/srcg/conftestqicollect/qicollect.conf -rrdpath=/tmp/badqi -debugout -port 1365  2>&1 | unbuffer -p tr ':' '='
	##	    ## # ./crtelnet 127.0.0.1:1364 localhost.key.priv
	##	    ## # exit 1
	##	    ## ### ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &
	##	    killall qigong || true
	##	    ## # ./qigong           -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364
	##	     ./qigong   -debugcrypt -nofork -localkey=`pwd`/localhost.key.priv       -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 2>&1   &
	##	    rm -f *.rrd
	##	    ./qicollect -debugcrypt -keydir=. -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 
	##	    ## # ./qicollect -pidfile=/tmp/qicollbuild.pid -logfile=testqicoll.log -conffile=test.conf -rrdpath=`pwd` -nofork -debugconnect -debugccstates -port 1365 && ( telnet localhost 1364 ; tail testqigong.log ) 


localhost.key.priv: qigenkey
	rm -f localhost.key.priv
	./qigenkey localhost

qigong.rc: qigong.rc.proto
	sed "s=@@PREFIX@@=${DESTDIR}=g" < qigong.rc.proto > qigong.rc

qicollect.rc: qicollect.rc.proto
	sed "s=@@PREFIX@@=${DESTDIR}=g" < qicollect.rc.proto > qicollect.rc

install_qigong: qigong qigong.rc qigenkey crtelnet
	ls -ld `pwd`/*
	strip qigong
	strip qigenkey
	strip crtelnet
	chmod 700 ./installscript
	./installscript "${DESTDIR}" qigong "${rcDESTDIR}"

install_qigong_nomc: qigong_nomc qigong.rc qigenkey crtelnet
	strip qigong_nomc
	strip qigenkey
	strip crtelnet
	cp -a qigong_nomc qigong
	./installscript "${DESTDIR}" qigong "${rcDESTDIR}"
	rm qigong

install_qicollect: qicollect qicollect.rc
	strip qicollect
	chmod 700 ./installscript
	./installscript "${DESTDIR}" qicollect "${rcDESTDIR}"

## this target is for maintainers
tarfiles.txt:
	repozclean
	( git archive `git rev-parse --abbrev-ref HEAD` | tar -tvf - | cut -b49- | grep -v debian-src ) > tarfiles.txt
	: > tstamp ; autoall ; find . -cnewer tstamp -not -type d | grep -v tarfiles.txt >> tarfiles.txt ; rm tstamp

tar: tarfiles.txt qicollect.hcpp qigong.cpp qiconn/include/qiconn/qiconn.h qiconn/qiconn.cpp
	tar -zcphf qigong_"${VERSION}".orig.tar.gz --no-recursion --transform='s=^='qigong_"${VERSION}"'/=' -T tarfiles.txt
	tar -ztvf qigong_"${VERSION}".orig.tar.gz



bin_PROGRAMS = qigong qicollect qigenkey crtelnet hex2base64 qigong_nomc

%.cc: %.hcpp
	${HCPP2CPP} $< -o $@

qicollect_SOURCES = qicollect.cc qicrypt.cpp qimeasure.cpp
qicollect_CXXFLAGS = @MYSQL_CLIENT_CFLAGS@ @LCF_LIBMEMCACHED@
qicollect_LDADD = @QICONNLDLIB@ @BULKRAYSLDLIB@
qicollect_LDFLAGS = @QICONNLDFLAGS@ @LIBMCRYPT_LIBS@ @MHASH_LIBS@ @BULKRAYSLDFLAGS@ @RRDLIB@ @MYSQL_CLIENT_LIBS@ @LDF_LIBMEMCACHED@

qigong_SOURCES = qigong.cpp qimeasure.cpp qicrypt.cpp
qigong_CXXFLAGS = @MYSQL_CLIENT_CFLAGS@ @LCF_LIBMEMCACHED@
qigong_LDADD = @QICONNLDLIB@
qigong_LDFLAGS = @QICONNLDFLAGS@ @LIBMCRYPT_LIBS@ @MHASH_LIBS@ @MYSQL_CLIENT_LIBS@ @MYSQL_CLIENT_LIBS@ @LDF_LIBMEMCACHED@

qigong_nomc_SOURCES = qigong.cpp qimeasure.cpp qicrypt.cpp
qigong_nomc_CXXFLAGS = @MYSQL_CLIENT_CFLAGS@
qigong_nomc_LDADD = @QICONNLDLIB@
qigong_nomc_LDFLAGS = @QICONNLDFLAGS@ @LIBMCRYPT_LIBS@ @MHASH_LIBS@ @MYSQL_CLIENT_LIBS@ @MYSQL_CLIENT_LIBS@


qigenkey_SOURCES = qigenkey.cpp qicrypt.cpp
qigenkey_LDADD = @QICONNLDLIB@ 
qigenkey_LDFLAGS = @QICONNLDFLAGS@ @LIBMCRYPT_LIBS@ @MHASH_LIBS@

crtelnet_SOURCES = crtelnet.cpp qicrypt.cpp
crtelnet_LDADD = @QICONNLDLIB@
crtelnet_LDFLAGS = @QICONNLDFLAGS@ @LIBMCRYPT_LIBS@ @MHASH_LIBS@

hex2base64_SOURCES = hex2base64.cpp qicrypt.cpp
hex2base64_LDADD = @QICONNLDLIB@  @LIBMCRYPT_LIBS@ @MHASH_LIBS@
hex2base64_LDFLAGS = @QICONNLDFLAGS@



CLEANFILES = qicollect.cc access_log

clean-local:
	rm -rf qigong-doc/*
	rm -f tarfiles.txt

distclean-local:
	rm m4/*.m4

