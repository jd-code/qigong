
all: qigong

vimtest: qigong
	# ./qicollect
	./qigong && telnet localhost 3307

qigong: qigong.o qiconn.o
	g++ -Wall -o qigong qigong.o qiconn.o

qigong.o: qigong.cpp qiconn.h qigong.h
	g++ -Wall -c qigong.cpp

#	qicollect: qicollect.cpp
#		g++ -Wall -o qicollect qicollect.cpp

qiconn.o: qiconn.cpp qiconn.h
	g++ -Wall -c qiconn.cpp

clean:
	rm -f qiconn.o qigong.o qigong qicollect 

