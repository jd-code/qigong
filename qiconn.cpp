#include <signal.h>

#include <iomanip>

#include "qiconn.h"

namespace qiconn
{
    using namespace std;

    bool debug_transmit = false; // debug all transmitions

    /*
     *  ---------------------------- simple ostream operators for hostent and sockaddr_in --------------------
     */

    ostream& operator<< (ostream& out, const struct hostent &he) {
	cout << he.h_name << " :" << endl;
	int i=0;
	if (he.h_addrtype == AF_INET) {
	    while (he.h_addr_list[i] != NULL) {
		out << (int)((unsigned char)he.h_addr_list[i][0]) << "."
		    << (int)((unsigned char)he.h_addr_list[i][1]) << "."
		    << (int)((unsigned char)he.h_addr_list[i][2]) << "."
		    << (int)((unsigned char)he.h_addr_list[i][3]) << endl;
		i++;
	    }
	} else {
	    cout << " h_addrtype not none" << endl ;
	}
	return out;
    }

    ostream& operator<< (ostream& out, struct sockaddr_in const &a) {
	const unsigned char* p = (const unsigned char*) &a.sin_addr;
	out << (int)p[0] << '.' << (int)p[1] << '.' << (int)p[2] << '.' << (int)p[3];
	return out;
    }

    /*
     *  ---------------------------- server_pool : opens a socket for listing a port at a whole --------------
     */

    #define MAX_QUEUDED_CONNECTIONS 5

    int server_pool (int port) {
	struct sockaddr_in serv_addr;

	memset (&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons (port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int s;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
	    int e = errno;
	    cerr << "could not create socket (for listenning connections :" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}

	{	int yes = 1;
	    if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) != 0) {
		int e = errno;
		cerr << "could not setsockopt (for listenning connections :" << port << ") : " << strerror (e) << endl ;
		return -1;
	    }
	}

	if (bind (s, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) != 0) {
	    int e = errno;
	    cerr << "could not bind socket (for listenning connections :" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}
	if (listen (s, MAX_QUEUDED_CONNECTIONS) != 0) {
	    int e = errno;
	    cerr << "could not listen socket (for listenning connections :" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}
	return s;
    }



    /*
     *  ---------------------------- init_connect : makes a telnet over socket, yes yes ----------------------
     */

    int init_connect (const char *fqdn, int port, struct sockaddr_in *ps /* = NULL */ ) {
	struct hostent * he;
	he = gethostbyname (fqdn);
	if (he != NULL) {
	    cout << *he;
	} else
	    return -1;		    // A logger et détailler JDJDJDJD

    //    struct servent *se = getservbyport (port, "tcp");
	
	struct sockaddr_in sin;
	bzero ((char *)&sin, sizeof(sin));

	sin.sin_family = AF_INET;
	bcopy (he->h_addr_list[0], (char *)&sin.sin_addr, he->h_length);
	sin.sin_port = htons(port);

	if (ps != NULL)
	    *ps = sin;
	
	struct protoent *pe = getprotobyname ("tcp");
	if (pe == NULL) {
	    int e = errno;
	    cerr << "could not get protocol entry tcp : " << strerror (e) << endl ;
	    return -1;
	}

	int s = socket(PF_INET, SOCK_STREAM, pe->p_proto);
	if (s == -1) {
	    int e = errno;
	    cerr << "could not create socket (for connection to " << fqdn << ":" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}

	if (connect (s, (const struct sockaddr *)&sin, sizeof(sin)) < 0) {
	    int e = errno;
	    cerr << "could not connect to " << fqdn << ":" << port << " : " << strerror (e) << endl ;
	    return -1;
	}
	else 
	    return s;
    }

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

    void Connection::close (void) {
	deregister_from_pool ();
	if (::close(fd) != 0) {
	    int e = errno;
	    cerr << "error closing fd[" << fd << "] : " << strerror(e) << endl ;
	}
    }

    void Connection::register_into_pool (ConnectionPool *cp) {
	if (cp != NULL) {
	    Connection::cp = cp;
	    cp->push (this);
	} else if (Connection::cp != cp)
	    cerr << "error: connection [" << getname() << "] already registered to another cp" << endl;
    }

    void Connection::deregister_from_pool () {
	if (cp != NULL) {
	    cp->pull (this);
	    cp = NULL;
	}
    }

    Connection::~Connection (void) {
	cerr << "destruction of fd[" << fd << "]" << endl ;
	deregister_from_pool ();
	close ();
    }

    void Connection::schedule_for_destruction (void) {
	if (cp != NULL)
	    cp->schedule_for_destruction (this);
	else
	    cerr << "warning : unable to register fd[" << getname() << "]for destrucion becasue cp=NULL" << endl;
    }


    SocketConnection::SocketConnection (int fd, struct sockaddr_in const &client_addr)
	: Connection (fd)
    {
	SocketConnection::client_addr = client_addr;
	stringstream s;
	s << client_addr;
	name = s.str();
    }

    /*
     *  ---------------------------- select pooling : need to be all in one class --------- JDJDJDJD ---------
     */

    /*
     *  ---------------------------- rought signal treatments ------------------------------------------------
     */

    int caught_signal;
    int pend_signals[256];

    void signal_handler (int sig) {
	printf ("got signal %d\n", sig);
	caught_signal ++;
	if ((sig > 0) && (sig<254))
	    pend_signals[sig]++;
	else
	    pend_signals[255]++;
    }

    int ConnectionPool::init_signal (void) {
	int i;
	int err = 0;
	for (i=0 ; i<256 ; i++)
	    pend_signals[i] = 0;
	caught_signal = 0;

	for (i=0 ; i<255 ; i++) {
	  if (i == 13)
	    if (SIG_ERR == signal(i, signal_handler)) {
		int e = errno;
		if (e != 22) {
		    cerr << "could not set signal handler for sig=" << i << " : [" << e << "]" << strerror (e) << endl;
		    err ++;
		}
	    } else {
		cerr << "signal [" << i << "] handled." << endl;
	    }
	}
	return err;
    }

    void ConnectionPool::treat_signal (void) {
	int i;
	for (i=0 ; i<256 ; i++) {
	    if (pend_signals [i] != 0) {
		cerr << "got sig[" << i << "] " << pend_signals [i] << " time" << ((i==1) ? "" : "s") << "." << endl;
		pend_signals [i] = 0;
	    }
	}
	caught_signal = 0;
    }

    ConnectionPool::ConnectionPool (void) {
	biggest_fd = -1;
	exitselect = false;
	scheddest = false;
    }


    void ConnectionPool::build_r_fd (void) {
	MConnections::iterator mi;
	FD_ZERO (&r_fd);
	for (mi=connections.begin() ; mi!=connections.end() ; mi++)
	    FD_SET (mi->first, &r_fd);
    }

    int ConnectionPool::set_biggest (void) {
	if (connections.empty())
	    biggest_fd = -1;
	else
	    biggest_fd = connections.begin()->first + 1;
	return biggest_fd;
    }

    void ConnectionPool::schedule_for_destruction (Connection * c) {
	if (connections.find (c->fd) == connections.end()) {
	    cerr << "warning: we were asked for destroying some unregistered connection[" << c->getname() << "]" << endl;
	    return;
	}
	destroy_schedule.push_back(c);
	scheddest = true;
    }

    int ConnectionPool::select_poll (struct timeval *timeout) {
	if (scheddest) {
	    list<Connection*>::iterator li;
	    for (li=destroy_schedule.begin() ; li!=destroy_schedule.end() ; li++)
		delete (*li);
	    destroy_schedule.erase (destroy_schedule.begin(), destroy_schedule.end());
	    scheddest = false;
	}

	MConnections::iterator mi;
	for (mi=connections.begin() ; mi!=connections.end() ; mi++)
	    mi->second->poll ();
	
	fd_set cr_fd = r_fd,
	       cw_fd = w_fd;
	select (biggest_fd, &cr_fd, &cw_fd, NULL, timeout);

	FD_ZERO (&w_fd);

	if (caught_signal)
	    treat_signal ();

	int i;
	for (i=0 ; i<biggest_fd ; i++) {
	    if (FD_ISSET(i, &cr_fd))
		connections[i]->read();
	}
	for (i=0 ; i<biggest_fd ; i++) {
	    if (FD_ISSET(i, &cw_fd))
		connections[i]->write();
	}
	if (exitselect)
	    return 1;
	else
	    return 0;
    }

    int ConnectionPool::select_loop (const struct timeval & timeout) {
	struct timeval t_out = timeout;
	build_r_fd ();
	FD_ZERO (&w_fd);

	while (select_poll(&t_out) == 0)  {
	    t_out = timeout;
	}
	return 0;
    }

    void ConnectionPool::closeall (void) {
	MConnections::reverse_iterator rmi;
	cerr << "entering ConnectionPool::closeall..." << endl;
	for (rmi=connections.rbegin() ; rmi!=connections.rend() ; rmi++) {
	    rmi->second->close();
	}
	cerr << "...done." << endl;
    }

    void ConnectionPool::destroyall (void) {
	MConnections::reverse_iterator rmi;
	cerr << "entering ConnectionPool::destroyall..." << endl;
	for (rmi=connections.rbegin() ; rmi!=connections.rend() ; rmi++) {
	    rmi->second->schedule_for_destruction();
	}
	cerr << "...done." << endl;
    }

    ConnectionPool::~ConnectionPool (void) {
	closeall ();
    }

    void ConnectionPool::push (Connection *c) {
	if (c->cp == NULL) {
	    c->cp = this;
	} else if (c->cp != this) {
	    cerr << "warning: connection[" << c->fd << ":" << c->getname() << "] already commited to another cp !" << endl;
	    return;
	}
	MConnections::iterator mi = connections.find (c->fd);
	if (mi == connections.end()) {
	    connections[c->fd] = c;
	    set_biggest ();
	    build_r_fd ();
	    reqw (c->fd);	/* we ask straightforwardly for write for welcome message */
	} else
	    cerr << "warning: connection[" << c->getname() << "] was already in pool ???" << endl;
    }

    void ConnectionPool::pull (Connection *c) {
	MConnections::iterator mi = connections.find (c->fd);
	if (mi != connections.end()) {
	    connections.erase (mi);
	    set_biggest ();
	    build_r_fd ();
	} else {
	    cerr << "warning: ConnectionPool::pull : cannot pull {" << c->getname() << "} !" << endl;
	}
    }


    /*
     *  ---------------------------- DummyConnection : connection that simply echo in hex what you just typed 
     */

    #define BUFLEN 1024
    DummyConnection::~DummyConnection (void) {
	ldums.erase (me);
    };

    DummyConnection::DummyConnection (int fd, struct sockaddr_in const &client_addr) : SocketConnection (fd, client_addr) {
	out = new stringstream ();
	wpos = 0;
	ldums.push_back (this);
	me = ldums.end();
	me--;
    }


    void DummyConnection::read (void) {
	char s[BUFLEN];
	ssize_t n = ::read (fd, (void *)s, BUFLEN);
	
	if (debug_transmit) {
	    int i;
	    clog << "fd=" << fd << "; ";
	    for (i=0 ; i<n ; i++)
		clog << s[i];
	    clog << endl;
	}
if (false)
{   int i;
    cerr << "DummyConnection::read got[";
    for (i=0 ; i<n ; i++)
	cerr << s[i];
    cerr << endl;
}
	int i;
	for (i=0 ; i<n ; i++) {
	    if ((s[i]==10) || (s[i]==13) || s[i]==0) {
		if (i+1<n) {
		    if ( ((s[i]==10) && (s[i+1]==13)) || ((s[i]==13) && (s[i+1]==10)) )
			i++;
		}
if (false) {
    cerr << "DummyConnection::read->lienread(" << bufin << ")" << endl;
}
		lineread ();
		bufin = "";
	    } else
		bufin += s[i];
	}
	if (n==0) {
	    cerr << "read() returned 0. we may close the fd[" << fd << "] ????" << endl;
	    // close ();
	    schedule_for_destruction();
	}
    }

    void DummyConnection::lineread (void) {
	string::iterator si;
	for (si=bufin.begin() ; si!=bufin.end() ; si++) {
	    (*out) << setw(2) << setbase(16) << setfill('0') << (int)(*si) << ':' ;
	}
	(*out) << " | " << bufin << endl;
	flush ();

	if (bufin.find("shut") == 0) {
	    if (cp != NULL) {
		cerr << "shutting down on request from fd[" << getname() << "]" << endl;
		cp->tikkle();
		// cp->closeall();
		// exit (0);
	    }
	    else
		cerr << "could not shut down from fd[" << getname() << "] : no cp registered" << endl;
	}

	if (bufin.find("wall ") == 0) {
	    list<DummyConnection *>::iterator li;
	    int i = 0;
	    for (li=ldums.begin() ; li!=ldums.end() ; li++) {
		(*(*li)->out) << i << " : " << bufin.substr(5) << endl;
		(*li)->flush();
		i++;
	    }
	}
    }

    void DummyConnection::flush(void) {
	if (cp != NULL)
	    cp->reqw (fd);
	bufout += out->str();
cerr << "                                                                                      ->out=" << out->str() << endl ;
	delete (out); 
	out = new stringstream ();
    }

    void DummyConnection::write (void) {
	ssize_t size = (ssize_t)bufout.size() - wpos,
	       nb;
	if (size == 0)
	    return;

	if (size > BUFLEN) {
	    cp->reqw (fd);
	    nb = BUFLEN;
	} else {
	    nb = size;
	}
	ssize_t nt = ::write (fd, bufout.c_str()+wpos, nb);
	if (nt != -1) {
	    wpos += nt;
	    if (nt != nb) {
		cerr << "some pending chars" << endl;
		cp->reqw (fd);
	    }
	} else {
	    int e = errno;

	    if (e == EPIPE) {   /* we can assume the connection is shut (and wasn't detected by read) */
		// close ();
		schedule_for_destruction();
	    } else {
		cerr << "error writing to fd[" << fd << ":" << getname() << "] : (" << e << ") " << strerror (e) << endl ;
	    }
	}
	if (wpos == bufout.size()) {
	    wpos = 0;
	    bufout = "";
	}
    }

    /*
     *  ---------------------------- ListeningSocket : the fd that watches incoming cnx ----------------------
     */


    int ListeningSocket::addconnect (int socket) {
	struct sockaddr_in client_addr;
	socklen_t size_addr = sizeof(client_addr);
	int f = accept ( socket, (struct sockaddr *) &client_addr, &size_addr );
	if (f < 0) {
	    int e = errno;
	    cerr << "could not accept connection : " << strerror (e) << endl ;
	    return -1;
	}
	cerr << "new connection from fd[" << f << ":" << client_addr << "]" << endl;
	if (cp != NULL) {
	    DummyConnection * pdc = connection_binder (f, client_addr);
	    if (pdc == NULL) {
		cerr << "error: could not add connection : failed to allocate DummyConnection" << endl;
		return -1;
	    }
	    cp->push (pdc);
	} else
	    cerr << "error: could not add connection : no cp registered yet !" << endl;
	// connections[fd] = new DummyConnection (fd, client_addr);
	// cout << "now on, biggest_fd = " << set_biggest () << " / " << connections.size() << endl;
	// build_r_fd ();
	return 0;
    }

    ListeningSocket::~ListeningSocket (void) {}

    DummyConnection * ListeningSocket::connection_binder (int fd, struct sockaddr_in const &client_addr) {
	return new DummyConnection (fd, client_addr);
    }
    
    void ListeningSocket::setname (const string & name) {
	ListeningSocket::name = name;
    }

    ListeningSocket::ListeningSocket (int fd) : Connection(fd) {}

    void ListeningSocket::read (void) {
	addconnect (fd);
    }

    ListeningSocket::ListeningSocket (int fd, const string & name) : Connection(fd) {
	setname (name);
    }

    string ListeningSocket::getname (void) {
	return name;
    }

    void ListeningSocket::write (void) {}

    /*
     *  ---------------------------- all-purpose string (and more) utilities ---------------------------------
     */

    bool getstring (istream & cin, string &s, size_t maxsize /* = 2048 */) {
	char c;
	size_t n = 0;
	while ((n < maxsize) && cin.get(c) && (c != 10) && (c != 13))
	    s += c, n++;
	return (cin);
    }

    size_t seekspace (const string &s, size_t p /* = 0 */) {
	if (p == string::npos)
	    return string::npos;

	size_t l = s.size();
	
	if (p >= l)
	    return string::npos;

	while ((p < l) && isspace (s[p])) p++;

	if (p >= l)
	    return string::npos;

	return p;
    }

    size_t getidentifier (const string &s, string &ident, size_t p /* = 0 */ ) {
	size_t l = s.size();
	ident = "";

	p = seekspace(s, p);
	
	if (p == string::npos)
	    return string::npos;
	
	if (p >= l)
	    return string::npos;

	if (! isalpha(s[p]))
	    return p;

	while (p < l) {
	    if (isalnum (s[p]) || (s[p]=='_')) {
		ident += s[p];
		p++;
	    } else
		break;
	}

	if (p >= l)
	    return string::npos;

	return p;
    }

    size_t getfqdn (const string &s, string &ident, size_t p /* = 0 */ ) {
	size_t l = s.size();
	ident = "";

	p = seekspace(s, p);
	
	if (p == string::npos)
	    return string::npos;
	
	if (p >= l)
	    return string::npos;

	if (! isalnum(s[p]))
	    return p;

	while (p < l) {
	    if (isalnum (s[p]) || (s[p]=='.') || (s[p]=='-')) {
		ident += s[p];
		p++;
	    } else
		break;
	}

	if (p >= l)
	    return string::npos;

	return p;
    }

    size_t getinteger (const string &s, long &n, size_t p /* = 0 */ ) {
	size_t l = s.size();
	string buf;

	p = seekspace(s, p);
	
	if (p == string::npos)
	    return string::npos;
	
	if (p >= l)
	    return string::npos;

	if (! isdigit(s[p]))
	    return p;

	while (p < l) {
	    if (isdigit (s[p])) {
		buf += s[p];
		p++;
	    } else
		break;
	}

	n = atol (buf.c_str());
	
	if (p >= l)
	    return string::npos;

	return p;
    }

    CharPP::CharPP (string const & s) {
	isgood = false;
	size_t size = s.size();
	size_t p;
	n = 0;
	char *buf = (char *) malloc (size+1);
	if (buf == NULL)
	    return;
	
	list <char *> lp;
	lp.push_back (buf);
	for (p=0 ; p<size ; p++) {
	    if (s[p] == 0)
		lp.push_back (buf + p + 1);
	    buf[p] = s[p];
	}
	n = lp.size() - 1;
	charpp = (char **) malloc ((n+1) * sizeof (char *));
	if (charpp == NULL) {
	    delete (buf);
	    n = 0;
	    return;
	}
	list <char *>::iterator li;
	int i;
	for (li=lp.begin(), i=0 ; (li!=lp.end()) && (i<n) ; li++, i++)
	    charpp[i] = *li;
	charpp[i] = NULL;
	isgood = true;
    }
    char ** CharPP::get_charpp (void) {
	if (isgood)
	    return charpp;
	else
	    return NULL;
    }
    size_t CharPP::size (void) {
	return n;
    }
    CharPP::~CharPP (void) {
	if (charpp != NULL) {
	    if (n>0)
		delete (charpp[0]);
	    delete (charpp);
	}
    }
    ostream& CharPP::dump (ostream& cout) const {
	int i = 0;
	while (charpp[i] != NULL)
	    cout << "    " << charpp[i++] << endl;
	return cout;
    }

    ostream & operator<< (ostream& cout, CharPP const & chpp) {
	return chpp.dump (cout);
    }
    
} // namespace qiconn

