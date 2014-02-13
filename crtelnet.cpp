
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <fstream>

#define QICONN_H_GLOBINST
#include "qicrypt.h"

using namespace std;
using namespace qiconn;

void usage (ostream &cout) {
    cout << "crtelnet dest_addrr[:port] keyfile" << endl;
}


class OurPool : public ConnectionPool {
    public:
	OurPool(void) : ConnectionPool () {}
	virtual ~OurPool() {}
	void askforexit (const char * reason);
};

void OurPool::askforexit (const char * reason) {
    exitselect = true;
    cerr << "ending : " << reason << endl;
}

class InteractivConn : public CryptConnection {
    InteractivConn ** extref;
    OurPool *pool;
  public:
    InteractivConn (int fd, struct sockaddr_in const &client_addr, const QiCrKey* qicrkey, OurPool *pool);
    virtual void lineread (void);

    inline virtual string getname (void) { return "InteractivConn"; }
    virtual const char * gettype (void) { return "InteractivConn"; }
    virtual ~InteractivConn (void);
    void setextref (InteractivConn ** x) { extref = x; }
};

InteractivConn::~InteractivConn (void) {
    if (extref != NULL) {
	*extref = NULL;
    }
    pool->askforexit("[connection closed]");
}

InteractivConn::InteractivConn (int fd, struct sockaddr_in const &client_addr, const QiCrKey* qicrkey, OurPool *pool)
    : CryptConnection (fd, client_addr, qicrkey), extref(NULL), pool(pool)
    {}

void InteractivConn::lineread (void) {
    cout << bufin << endl;
}

class ConsoleHook : public BufConnection {
    InteractivConn *remote;
  public:
    ConsoleHook (int fd, InteractivConn *remote);
    virtual void lineread (void);

    inline virtual string getname (void) { return "ConsoleHook"; }
    virtual const char * gettype (void) { return "ConsoleHook"; }
};

ConsoleHook::ConsoleHook (int fd, InteractivConn *remote)
    : BufConnection (fd), remote(remote) {
    if (remote != NULL)
	remote->setextref (&(this->remote));
}

void ConsoleHook::lineread (void) {
    if (remote == NULL) return;
    (*remote->out) << bufin << endl;
    remote->flush();
}
int main (int nb, char ** cmde) {

    if (nb < 3) {
	usage (cout);
	return 1;
    }

    string fulldest(cmde[1]);

    QiCrKey qicrkey(cmde[2]);

//    string key;
//    {	ifstream fkey(cmde[2]);
//	if (!fkey) {
//	    int e = errno;
//	    cerr << "could not open file " << cmde[2] << " : " << strerror(e) << endl;
//	    return -1;
//	}
//	while (fkey) {
//	    key += fkey.get();
//	}
//    }

    int port;
    string dest;
    struct sockaddr_in so_in;

    size_t p = fulldest.find(':');
    if (p != string::npos) {
	dest.assign (fulldest.substr(0,p));
	port = atoi (fulldest.substr(p+1).c_str());
    } else {
	dest.assign (fulldest);
	port = 1264;
    }

    int sock;

    time_t ctimeout = 10;
    time_t start = time(NULL);

    while (	((sock = init_connect (dest.c_str(), port, &so_in)) <0)
	    &&  (errno == EINPROGRESS)
	    &&	((time(NULL)-start) < ctimeout)) {}

    if (sock < 0) {
	int e = errno;
	if (e == EINPROGRESS) {
	    cerr << "could not connect to " << dest << ":" << port << " : connection attempt timed out" << endl;

	} else {
	    cerr << "could not connect to " << dest << ":" << port << " : " << strerror(e) << endl;
	}
	return 1;
    }

    OurPool cp;
    cp.init_signal ();


    InteractivConn *remote = new InteractivConn(sock, so_in, &qicrkey, &cp);
    if (remote == NULL) {
	cerr << "could not allocate InteractivConn" << endl;
	::close (sock);
	return 1;
    }

    ConsoleHook *consolehook = new ConsoleHook(0, remote);
    if (consolehook == NULL) {
	cerr << "could not allocate console hook" << endl;
	delete (remote);
	::close (sock);
	return 1;
    }

//    remote.register_into_pool (cp);
    cp.push(remote);
    cp.push(consolehook);

    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 25000;

    cp.select_loop (timeout);

    cp.closeall();

    return 0;
}

