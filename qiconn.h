#ifndef QICONN_H_INCLUDE
#define QICONN_H_INCLUDE

#ifdef QICONN_H_GLOBINST
#define QICONN_H_SCOPE
#else
#define QICONN_H_SCOPE extern
#endif


#include <netdb.h>	    // gethostbyname
// #include <sys/types.h>
// #include <sys/socket.h>	    // connect

#include <iostream>
#include <sstream>
#include <list>
#include <map>

#define QICONNPORT 1264

namespace qiconn
{
    using namespace std;

#ifdef QICONN_H_GLOBINST
    QICONN_H_SCOPE bool debug_resolver = false;	    // debug init_connect resolver
    QICONN_H_SCOPE bool debug_transmit = false;	    // debug all transmitions
    QICONN_H_SCOPE bool debug_dummyin = false;	    // debug input of dummyconn
    QICONN_H_SCOPE bool debug_lineread = false;	    // debug lineread of dummyconn
    QICONN_H_SCOPE bool debug_dummyout = false;	    // debug output of dummyconn
#else
    QICONN_H_SCOPE bool debug_resolver;		    // debug init_connect resolver
    QICONN_H_SCOPE bool debug_transmit;		    // debug all transmitions
    QICONN_H_SCOPE bool debug_dummyin;		    // debug input of dummyconn
    QICONN_H_SCOPE bool debug_lineread;		    // debug lineread of dummyconn
    QICONN_H_SCOPE bool debug_dummyout;		    // debug output of dummyconn
#endif

    /*
     *  ---------------------------- simple ostream operators for hostent and sockaddr_in --------------------
     */

    ostream& operator<< (ostream& out, const struct hostent &he);
    ostream& operator<< (ostream& out, struct sockaddr_in const &a);

    /*
     *  ---------------------------- server_pool : opens a socket for listing a port at a whole --------------
     */


    int server_pool (int port);

    /*
     *  ---------------------------- init_connect : makes a telnet over socket, yes yes ----------------------
     */

    int init_connect (const char *fqdn, int port, struct sockaddr_in *ps = NULL);

    /*
     *  ---------------------------- init_connect : makes a telnet over socket, yes yes ----------------------
     */

    class FQDNPort {
	public:
	    string fqdn;
	    int port;
	    inline FQDNPort (string fqdn = "", int port = 0) {
		FQDNPort::fqdn = fqdn,
		FQDNPort::port = port;
	    }
	    inline bool operator< (const FQDNPort & fp) const {
		if (port != fp.port)
		    return port < fp.port ;
		else
		    return fqdn < fp.fqdn ;
	    }
    };
    
    /*
     *  ---------------------------- Connectionl : handles an incoming connection from fd --------------------
     */

    /*
     *	virtual class in order to pool and watch several fds with select later on.
     *	this skeleton provides
     *	    building from a file descriptor and an incoming internet socket address
     *	    building from a file descriptor only (partly broken)
     *	    read is called whene theres some reading found by select
     *	    write is called when the fd requested to watch for write status and is write-ready
     */

    class ConnectionPool;

    class Connection
    {
	friend class ConnectionPool;
	protected:
	    int fd;
	    ConnectionPool *cp;

	public:
	    virtual ~Connection (void);
	    //  Connection () {
	    //      fd = -1;
	    //      cp = NULL;
	    //  }
	    inline Connection (int fd) {
		Connection::fd = fd;
		cp = NULL;
	    }
	    virtual void read (void) = NULL;
	    virtual void write (void) = NULL;
	    virtual string getname (void) = NULL;
	    void register_into_pool (ConnectionPool *cp);
	    void deregister_from_pool ();
	    void close (void);
	    void schedule_for_destruction (void);
	    virtual void poll (void) = NULL;
    };

    class SocketConnection : public Connection
    {
	protected:
	                           struct sockaddr_in client_addr;
	                           string name;
	public:
	        virtual ~SocketConnection (void) { cerr << "destruction of fd[" << fd << ", " << name << "]" << endl; }
	                 SocketConnection (int fd, struct sockaddr_in const &client_addr);
	             virtual void setname (struct sockaddr_in const &client_addr);
	    inline virtual string getname (void) {
		return name;
	    }
    };

    /*
     *  ---------------------------- select pooling : polls a pool of connection via select ------------------
     */


    class ConnectionPool
    {
	private:
	    bool exitselect;
	    
	    struct gtint
	    {
		inline bool operator()(int s1, int s2) const {
		    return s1 > s2;
		}
	    };

	    typedef map <int, Connection*, gtint> MConnections;
	    
	    MConnections connections;
	    list<Connection*> destroy_schedule;
	    bool scheddest;

	    fd_set opened;
	    fd_set r_fd;
	    fd_set w_fd;

	    int biggest_fd;

	    void build_r_fd (void);

	    int set_biggest (void);

	    /*
	     *  -------------------- rought signal treatments ------------------------------------------------
	     */

	    void treat_signal (void);


	public:
	    ConnectionPool (void);
	    int add_signal_handler (int sig);
	    int remove_signal_handler (int sig);
	    int init_signal (void);

	    inline void tikkle (void) { exitselect = true; }
	    
	    void schedule_for_destruction (Connection * c);

	    virtual int select_poll (struct timeval *timeout);

	    int select_loop (const struct timeval & timeout);

	    void closeall (void);

	    void destroyall (void);

	    /* request no read (for files select always return read available)  */
	    inline void reqnor (int fd) { if (fd >= 0) FD_CLR (fd, &r_fd); }
	    /* request reading */
	    inline void reqr (int fd) { if (fd >= 0) FD_SET (fd, &r_fd); }
	    /* request writing */
	    inline void reqw (int fd) { if (fd >= 0) FD_SET (fd, &w_fd); }
	    
	    virtual ~ConnectionPool (void);

	    void push (Connection *c);

	    void pull (Connection *c);
    };

    /*
     *  ---------------------------- DummyConnection : connection that simply echo in hex what you just typed 
     */

    class DummyConnection : public SocketConnection
    {
	private:
					 string	bufout;
					 size_t	wpos;
		 static list<DummyConnection *>	ldums;
	      list<DummyConnection *>::iterator	me;
	protected:
					 string	bufin;
	public:
				   stringstream	*out;
					virtual	~DummyConnection (void);
						DummyConnection (int fd, struct sockaddr_in const &client_addr);
				   virtual void	read (void);
				   virtual void	lineread (void);
					   void	flush(void);
				   virtual void	write (void);
				   virtual void poll (void) {}
				   virtual void reconnect_hook (void);
    };

    #ifdef QICONN_H_GLOBINST
	list<DummyConnection *> DummyConnection::ldums;
    #endif

    /*
     *  ---------------------------- ListeningSocket : the fd that watches incoming cnx ----------------------
     */

    class ListeningSocket : public Connection
    {
	private:
			     string name;
				int addconnect (int socket);
	public:
			    virtual ~ListeningSocket (void);
			       void setname (const string & name);
				    ListeningSocket (int fd);
				    ListeningSocket (int fd, const string & name);
	   virtual DummyConnection* connection_binder (int fd, struct sockaddr_in const &client_addr);
		     virtual string getname (void);
		       virtual void read (void);
		       virtual void write (void);
    };
    
    /*
     *  ---------------------------- all-purpose string (and more) utilities ---------------------------------
     */

    bool getstring (istream & cin, string &s, size_t maxsize = 2048);
    size_t seekspace (const string &s, size_t p = 0);
    size_t getidentifier (const string &s, string &ident, size_t p = 0);
    size_t getfqdn (const string &s, string &ident, size_t p = 0 );
    size_t getinteger (const string &s, long &n, size_t p = 0);
    size_t getinteger (const string &s, long long &n, size_t p = 0);

    inline char eos(void) { return '\0'; }

    class CharPP {
	private:
	    char ** charpp;
	    int n;
	    bool isgood;

	public:
	    CharPP (string const & s);
	    char ** get_charpp (void);
	    size_t size (void);
	    ~CharPP (void);
	    ostream& dump (ostream& cout) const;
    };
    ostream & operator<< (ostream& cout, CharPP const & chpp);
    

}   // namespace qiconn

#endif // QICONN_H_INCLUDE
