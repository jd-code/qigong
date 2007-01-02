#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fstream>
#include <iomanip>

//#define QICONN_H_GLOBINST
//#define QIGONG_H_GLOBINST
//#include "qigong.h"
#include "qiconn.h"
#include "qimeasure.h"

namespace qiconn {
    using namespace std;

    ostream& operator<< (ostream& cout, const MeasurePoint& mp) {
	mp.dump(cout);
	return cout;
    }

/*
 *  ---- MPdiskstats ---------------------------------------------------------------------------------------
 */

    MPdiskstats::MPdiskstats (const string & param) : MeasurePoint (param) {
	name="diskstats";
	size_t p = param.find(' ');
	if (p == string::npos) {
	    diskname = param;
	    field = 7;		// written sectors ?? JDJDJDJD
// cerr << "diskname = <" << diskname << ">   field = <" << field << ">" << endl ;
	    return;
	}
	diskname = param.substr (0, p);
	field = atoi (param.substr (p).c_str());
// cerr << "diskname = <" << diskname << ">   field = <" << field << ">" << endl ;
    }

    bool MPdiskstats::proc_diskstats (string & result) {
	string s;
	ifstream ds("/proc/diskstats");
	size_t l = diskname.size();
	while (getstring (ds, s)) {
	    if (s.substr (10, l) == diskname) {
// cerr << s << endl ;
		size_t i;
		size_t p=10;
		for (i=0 ; i<field ; i++) {
		    p = s.find (' ', p);
		    if (p == string::npos)
			break;
		    p++;
		}
		if (p!=string::npos) {
		    result = s.substr(p, s.find(' ',p) - p);
		    return true;
		    // return (atol (s.substr(p).c_str()));
		} else {
		    return false;
		}
	    }
	    s = "";
	}
	return false;
    }

    bool MPdiskstats::measure (string &result) {
	return proc_diskstats (result);
    }

    MeasurePoint* MPdiskstats_creator (const string & param) {
	return new MPdiskstats (param);
    }


/*
 *  ---- MPnetstats ---------------------------------------------------------------------------------------
 */

    MPnetstats::MPnetstats (const string & param) : MeasurePoint (param) {
	name="netstats";
	size_t p = param.find(' ');
	if (p == string::npos) {
	    intname = param;
	    intname += ':';
	    field = 7;		// written sectors ?? JDJDJDJD
	    return;
	}
	intname = param.substr (0, p);
	intname += ':';
	field = atoi (param.substr (p).c_str());
    }

    bool MPnetstats::proc_net_dev (string & result) {
	string s;
	ifstream ds("/proc/net/dev");
	while (getstring (ds, s)) {
	    size_t p = s.find (intname);
	    if (p != string::npos) {
// cerr << s << endl ;
		p += intname.size();
		p = seekspace (s, p);
		size_t i;
		for (i=0 ; i<field ; i++) {
		    long long dummy;
		    p = getinteger (s, dummy, p);
		    if (p == string::npos)
			break;
		    p = seekspace (s, p);
		    if (p == string::npos)
			break;
		}
		if (p!=string::npos) {
		    result = s.substr(p, s.find(' ',p) - p);
		    return true;
		    // return (atol (s.substr(p).c_str()));
		} else {
		    return false;
		}
	    }
	    s = "";
	}
	return false;
    }

    bool MPnetstats::measure (string &result) {
	return proc_net_dev (result);
    }

    MeasurePoint* MPnetstats_creator (const string & param) {
	return new MPnetstats (param);
    }


/*
 *  ---- MPfilelen -----------------------------------------------------------------------------------------
 */

    class LogCountConn : public Connection
    {
#define LCC_BUFLEN 4096
	protected:
	    long long nl;
	    string fname;
	    off_t prevsize;
	public:
	    virtual ~LogCountConn (void) {}
	    LogCountConn (const string &fname, int fd) : Connection (fd) {
		LogCountConn::fname = fname;
		nl = 0;
		prevsize = 0;
	    }
	    virtual void read (void) {
		char s[LCC_BUFLEN];
		ssize_t n = LCC_BUFLEN;
		int i;
		    while (n == LCC_BUFLEN) {
			n = ::read (fd, (void *)s, LCC_BUFLEN);
//    if (n==0) cerr << "read 0 b" << ((long) time(NULL)) << endl;
			for (i=0 ; i<n ; i++) {
			    if ((s[i]==10) || (s[i]==13) || s[i]==0) {
				if (i+1<n) {
				    if ( ((s[i]==10) && (s[i+1]==13)) || ((s[i]==13) && (s[i+1]==10)) )
					i++;
				}
				nl++;
			    }
			}
		    }
		    if (cp != NULL)
			cp->reqnor(fd);
	    }
	    virtual void write (void) {}
	    virtual string getname (void) {
		return (fname);
	    }
	    virtual void poll (void) {
		struct stat newstat;
		fstat (fd, &newstat);
		if (newstat.st_size != prevsize) {
		    prevsize = newstat.st_size;
		    if (cp != NULL)
			cp->reqr(fd);
		}
	    }
	friend class MPfilelen;
    };
/*
 *  ---- MPfilelen -----------------------------------------------------------------------------------------
 */

    void MPfilelen::reopen (bool seekend) {
	fd = open (fname.c_str(), O_RDONLY);
	plcc = NULL;

cerr << "MPfilelen::reopen (" << fname << ") = " << fd << endl;
	
	if (fd == -1)
	    return;

	if (fstat (fd, &curstat) == -1) {
cerr << "MPfilelen::reopen fstat(" << fd << ") = -1" << endl;
	    return;
	}

	if (seekend)
	    lseek (fd, 0, SEEK_END);
	
	if (pcp != NULL) {
	    plcc = new LogCountConn (fname, fd);
	    if (plcc != NULL) {
		if (!seekend)
		    plcc->read();
		pcp->push(plcc);
	    } else {
		cerr << "MPfilelen::reopen : could not allocate LogCountConn (" << fname << "," << fd << ")" << endl;
		close (fd);
		fd = -1;
	    }
	}

    }
    
    MPfilelen::~MPfilelen (void) {
	if (plcc != NULL) delete (plcc);
	plcc = NULL;
    }
    
    MPfilelen::MPfilelen (const string & param) : MeasurePoint (param) {
	name="filelen";
	fname = param;
	reopen (true);
    }

    bool MPfilelen::measure (string &result) {
	stringstream out;
	if (plcc == NULL) {
	    reopen (false);
	    if (plcc == NULL)
		out << "U";
	    else {
		oldnl = plcc->nl;
		out << oldnl;
	    }
	} else if (oldnl == plcc->nl) {
	    struct stat newstat;
	    lstat (fname.c_str(), &newstat);
	    if ((newstat.st_dev != curstat.st_dev) || (newstat.st_ino != curstat.st_ino)) {
		if (plcc != NULL) delete (plcc);
		reopen (false);
		if (plcc == NULL)
		    out << "U";
		else {
		    oldnl = plcc->nl;
		    out << oldnl;
		}
	    } else {
		out << oldnl;
		lseek (fd, 0, SEEK_END);
	    }
	} else {
	    oldnl = plcc->nl;
	    out << oldnl;
	}
	result = out.str();
	return true;
    }

    MeasurePoint* MPfilelen_creator (const string & param) {
	return new MPfilelen (param);
    }

    ConnectionPool *MPfilelen::pcp = NULL;
/*
 *  ---- initialisation of MPs -----------------------------------------------------------------------------
 */

    void init_mmpcreators (ConnectionPool *pcp) {
	mmpcreators["diskstats"]    = MPdiskstats_creator;
	mmpcreators["netstats"]	    = MPnetstats_creator;
	mmpcreators["filelen"]	    = MPfilelen_creator;
	MPfilelen::pcp = pcp;
    }

} // namespace qiconn
