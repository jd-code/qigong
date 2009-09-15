#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> // strerror ??
#include <stdlib.h> // atoi ??
#include <sys/statvfs.h>
#include <dirent.h>

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
 *  ---- get_most_recent -----------------------------------------------------------------------------------
 */

    int GetMostRecent::matchentry (const string &fname, struct stat const & fst) {
	return S_ISREG (fst.st_mode);
    }

    GetMostRecent::GetMostRecent () {}

    GetMostRecent::GetMostRecent (const string & dirname) {
	GetMostRecent::dirname = dirname;
    }

    bool GetMostRecent::getlast (string & flast) {
	DIR * dir;

	dir = opendir (dirname.c_str());
	if (dir == NULL) {
	    int e = errno;
	    cerr << "could not opendir (\"" << dirname << "\") : " << strerror (e) << endl;
	    return 0;
	}
	
	struct dirent * pde;
	time_t tlast = 0;
	bool found = false;
	while ((pde = readdir (dir)) != NULL) {
	    string fname = dirname;
	    fname += "/";
	    fname += pde->d_name;

	    struct stat fst;
	    if (lstat (fname.c_str(), &fst) != 0) {
		int e = errno;
		cerr << "could not stat (\"" << fname << "\") : " << strerror (e) << endl;
		continue;
	    }
	    if (matchentry (pde->d_name, fst) && (fst.st_mtime >= tlast)) {
		found = true;
		flast = fname;
		tlast = fst.st_mtime;
	    }
	}
	
	closedir (dir);
	return found;
    }

    int MatchBeginEnd::matchentry (const string &fname, struct stat const & fst) {
	if (!S_ISREG (fst.st_mode))
	    return 0;
	if (fname.find(fname_begin) == 0) {
	    if (fname_end.empty())
		return 1;
	    size_t p = fname.rfind(fname_end);
	    if ((p != string::npos) && (p == fname.size() - fname_end.size()))
		return 1;
	}
	return 0;
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
	    size_t p = 0, max = s.size();
	    {	int i;	// we must skip two digits fields prepended with spaces (which width changed upon time)
		for (i=0 ; i<2 ; i++) {
		    while ((p<max) && (isspace(s[p]))) p++;
		    while ((p<max) && (isdigit(s[p]))) p++;
		}
		p++;
	    }
	    if (s.substr (p, l) == diskname) {
// cerr << s << endl ;
		size_t i;
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
 *  ---- LogCountConn --------------------------------------------------------------------------------------
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

if (!fname.empty()) cerr << "MPfilelen::reopen (" << fname << ") = " << fd << endl;
	
	if (fd == -1)
	    return;

	if (fstat (fd, &curstat) == -1) {
cerr << "MPfilelen::reopen fstat(" << fd << ") = -1" << endl;
	    return;
	}

	tlast_nonzero = time(NULL);

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
    
    MPfilelen::MPfilelen (const string & param, time_t zltimeout /* = 0 */) : MeasurePoint (param) {
	oldnl = 0;
	MPfilelen::zltimeout = zltimeout;
	tlast_nonzero = time(NULL);
	name = "filelen";
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
		if (zltimeout != 0) {
		    tlast_nonzero = time(NULL);
		}
		oldnl = plcc->nl;
		out << oldnl;
	    }
	} else if (oldnl == plcc->nl) {
	    struct stat newstat;
	    lstat (fname.c_str(), &newstat);
	    if (    ((zltimeout != 0) && ((time(NULL)-tlast_nonzero) > zltimeout))
		 || (newstat.st_dev != curstat.st_dev)
		 || (newstat.st_ino != curstat.st_ino)
	       ) {
		if (plcc != NULL) delete (plcc);
		plcc = NULL;
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
	    if (zltimeout != 0) {
		tlast_nonzero = time(NULL);
	    }
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
 *  ---- MPlastmatchfilelen --------------------------------------------------------------------------------
 */

    MPlastmatchfilelen::~MPlastmatchfilelen (void) {
	if (plcc != NULL) delete (plcc);
	plcc = NULL;
    }
    
    MPlastmatchfilelen::MPlastmatchfilelen (const string & param) : MPfilelen ("", 30) {
	name="matchflen";
	oldnl = 0;
	size_t p = param.find(',');
	getmostrecent.dirname = param.substr(0,p);
	if (p != string::npos) {
	    size_t q = p + 1;
	    p = param.find(',', q);
	    if (p != string::npos)
	    getmostrecent.fname_begin = param.substr(q, (p != string::npos) ? p-q: string::npos);
	}
	if (p != string::npos) {
	    size_t q = p + 1;
	    p = param.find(',', q);
	    getmostrecent.fname_end = param.substr(q, (p != string::npos) ? p-q: string::npos);
	}
//	cerr << "dirname = \"" << getmostrecent.dirname
//	     << "\", fname_begin = \"" << getmostrecent.fname_begin
//	     << "\", fname_end = \"" << getmostrecent.fname_end << "\"" << endl;

	reopen (true);
    }

    void MPlastmatchfilelen::reopen (bool seekend) {
	if (!getmostrecent.getlast (fname)) {
	    fname.erase();
	    fd = -1;
cerr << "MPlastmatchfilelen::reopen (" << getmostrecent.dirname << "/" << getmostrecent.fname_begin << "*" << getmostrecent.fname_end << ") = no match found" << endl;
	    return;
	}
	MPfilelen::reopen (true);
    }
    
    MeasurePoint* MPlastmatchfilelen_creator (const string & param) {
	return new MPlastmatchfilelen (param);
    }


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
static const char *s[] = {"free", "buffers", "cached", "used", "sw_used", "sw_free"};
	if ((i>=0) && (i<6))
	    return s[i];
	else
	    return "";
    }
    
    string MPmeminfo::get_next_rras (int i) {
// static char *s[] = {"free", "buffers", "used", "sw_used", "sw_free"};
static const char *s[] =    {"MIN", "MAX", "MAX", "MAX", "MAX", "MIN"};
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
	    s << ((long long)buf.f_frsize * buf.f_bavail) ;
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
 *  ---- MPconncount -----------------------------------------------------------------------------------------
 */

    MPconncount::~MPconncount (void) {
    }
    
    MPconncount::MPconncount (const string & param) : MeasurePoint (param) {
    }

    bool MPconncount::measure (string &result) {
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

    MeasurePoint* MPconncount_creator (const string & param) {
	return new MPconncount (param);
    }

/*
 *  ---- initialisation of MPs -----------------------------------------------------------------------------
 */

    void init_mmpcreators (ConnectionPool *pcp) {
	mmpcreators["diskstats"]    = MPdiskstats_creator;
	mmpcreators["netstats"]	    = MPnetstats_creator;
	mmpcreators["filelen"]	    = MPfilelen_creator;
	mmpcreators["matchflen"]    = MPlastmatchfilelen_creator;
	mmpcreators["loadavg"]	    = MPloadavg_creator;
	mmpcreators["conncount"]    = MPconncount_creator;
	mmpcreators["meminfo"]	    = MPmeminfo_creator;
	mmpcreators["freespace"]    = MPfreespace_creator;
	MPfilelen::pcp = pcp;
    }

} // namespace qiconn
