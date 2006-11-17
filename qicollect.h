#ifndef QICOLLECT_H_INCLUDE
#define QICOLLECT_H_INCLUDE

#ifdef QICOLLECT_H_GLOBINST
#define SCOPE
#else
#define SCOPE external
#endif

#include <fstream>
#include <iomanip>

#include "qiconn.h"

namespace qiconn {

    using namespace std;

#ifdef QICOLLECT_H_GLOBINST
    char * version = "Qicollect v0.5 - $Id:$";
#else
    external char * version;
#endif





    /*
     *  ------------------- the measuring polling system -----------------------------------------------------
     */

    class CollectPool : public ConnectionPool
    {
	private:
	public:
	    CollectPool (void) : ConnectionPool () {}

	    virtual int select_poll (struct timeval *timeout);
    };

    /*
     *  ------------------- the collecting connections -------------------------------------------------------
     */

    class CollectingConn : public DummyConnection
    {
	public:
	    virtual ~CollectingConn (void);
	    CollectingConn (int fd, struct sockaddr_in const &client_addr);
	    virtual void lineread (void);
    };


    class ListeningBinder : public ListeningSocket
    {
	public:
	    virtual ~ListeningBinder (void) {}
	    ListeningBinder (int fd) : ListeningSocket (fd, "socketbinder") {}
	    virtual DummyConnection* connection_binder (int fd, struct sockaddr_in const &client_addr) {
		return new CollectingConn (fd, client_addr);
	    }
    };


} // namespace qiconn

#endif // QICOLLECT_H_INCLUDE
