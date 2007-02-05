#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/statvfs.h>

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
	oldnl = 0;
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
 *  ---- MPloadavg -----------------------------------------------------------------------------------------
 */

    MPloadavg::~MPloadavg (void) {
    }
    
    MPloadavg::MPloadavg (const string & param) : MeasurePoint (param) {
    }

    bool MPloadavg::measure (string &result) {
	string s;
	ifstream loadavg ("/proc/loadavg");
	if (!loadavg) {
	    result = "U";
	    return true;
	}
	getstring (loadavg, s);
	size_t p = s.find(' ');
	if (p == string::npos) {
	    result = "U";
	    return true;
	}
	result = s.substr(0, p);
	return true;
    }

    MeasurePoint* MPloadavg_creator (const string & param) {
	return new MPloadavg (param);
    }

/*
 *  ---- MPmeminfo -----------------------------------------------------------------------------------------
 */

    MPmeminfo::~MPmeminfo (void) {
    }
    
    MPmeminfo::MPmeminfo (const string & param) : MeasureMultiPoint (param) {
	mss ["MemTotal:"] = 0;
	mss ["MemFree:"] = 0;
	mss ["Buffers:"] = 0;
	mss ["SwapTotal:"] = 0;
	mss ["SwapFree:"] = 0;
	mss ["Cached:"] = 0;
    }

    bool MPmeminfo::measure (string &result) {
	ifstream fmem ("/proc/meminfo");
	if (!fmem) {
	    result = "U:U:U:U:U";
	    return true;
	}
	while (fmem) {
	    string s;
	    getstring (fmem, s);
	    size_t p = s.find(' ');
	    string tag = s.substr(0, p);
	    map<string, long long>::iterator mi = mss.find(tag);
	    int match = 0;
	    if (mi != mss.end()) {
// cerr << s << endl;
		p = seekspace (s, p);
		long long v;
		getinteger (s, v, p);
		mi->second = v;
// cerr << "..." << tag << "..." << v << "..." << endl;
		match ++;
		if (match == 6) break;
	    }
	}
	
	stringstream out;
	out << mss ["MemFree:"] << ':' 
	    << mss ["Buffers:"] << ':'
	    << mss ["Cached:"] << ':'
	    << mss ["MemTotal:"] - mss ["Buffers:"] - mss ["MemFree:"] - mss ["Cached:"] << ':'
	    << mss ["SwapTotal:"] - mss ["SwapFree:"] << ':'
	    << mss ["SwapFree:"];
	result = out.str();
	return true;
    }

    MeasurePoint* MPmeminfo_creator (const string & param) {
	return new MPmeminfo (param);
    }

    int MPmeminfo::get_nbpoints (void) {
	return 6;
    }

    string MPmeminfo::get_tagsub (int i) {
static char *s[] = {"free", "buffers", "cached", "used", "sw_used", "sw_free"};
	if ((i>=0) && (i<6))
	    return s[i];
	else
	    return "";
    }
    
    string MPmeminfo::get_next_rras (int i) {
// static char *s[] = {"free", "buffers", "used", "sw_used", "sw_free"};
static char *s[] =    {"MIN", "MAX", "MAX", "MAX", "MAX", "MIN"};
	if ((i>=0) && (i<6))
	    return s[i];
	else
	    return "";
    }
    
/*
 *  ---- MPfreespace -----------------------------------------------------------------------------------------
 */

    MPfreespace::~MPfreespace (void) {
    }
    
    MPfreespace::MPfreespace (const string & param) : MeasurePoint (param) {
	name="freespace";
	fname = param;
	lasterr = 0;
	lasttimeerr = 0;
    }

    bool MPfreespace::measure (string &result) {
	stringstream s;
	struct statvfs buf;

	if (statvfs (fname.c_str(), &buf) == 0) {
	    s << buf.f_frsize * buf.f_bfree ;
	    result = s.str();
	    if (lasterr != 0) {
		lasterr = 0;
		lasttimeerr = 0;
	    }
	} else {
	    int e = errno;
	    result = "U";
	    if ((e != lasterr) || (time(NULL) - lasttimeerr > 600)) {
		lasterr = e;
		lasttimeerr = time(NULL);
		cerr << "error : statvfs(" << fname << ") : " << strerror (e) << endl;
	    }
	}
	return true;
    }

    MeasurePoint* MPfreespace_creator (const string & param) {
	return new MPfreespace (param);
    }

/*
 *  ---- initialisation of MPs -----------------------------------------------------------------------------
 */

    void init_mmpcreators (ConnectionPool *pcp) {
	mmpcreators["diskstats"]    = MPdiskstats_creator;
	mmpcreators["netstats"]	    = MPnetstats_creator;
	mmpcreators["filelen"]	    = MPfilelen_creator;
	mmpcreators["loadavg"]	    = MPloadavg_creator;
	mmpcreators["meminfo"]	    = MPmeminfo_creator;
	mmpcreators["freespace"]    = MPfreespace_creator;
	MPfilelen::pcp = pcp;
    }

} // namespace qiconn
