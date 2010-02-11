#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> // strerror ??
#include <stdlib.h> // atoi ??
#include <sys/statvfs.h>
#include <dirent.h>
#include <libmemcached/memcached.h>

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
 *  ---- MMemcached ----------------------------------------------------------------------------------------
 */

    MMemcached::~MMemcached (void) {
	if (mc != NULL)
	    memcached_free (mc);
    }
    
    MMemcached::MMemcached (const string & param) : MeasureMultiPoint (param) {
	name = "memcached";

	size_t p;
	bool foundport = false;
	p = param.find(':');
	if (p != string::npos) {
	    foundport = true;
	} else {
	    foundport = false;
	    p = param.find(',');
	}
	servername = param.substr (0, p);
	if (foundport) {
	    p++;
	    port = atoi (param.substr(p).c_str());
	    p = param.find(',');
	} else
	    port = 11211;
	
	if (p != string::npos) {
	    p++;
	    size_t q = param.find(',');
	    if (q != string::npos)
		source_type = param.substr(p, q-p);
	    else
		source_type = param.substr(p);
	} else
	    source_type = "DERIVE";

	mss ["curr_items"]            = "curr_items";
	mss ["total_items"]           = "total_items";
	mss ["bytes"]                 = "bytes";
	mss ["curr_connections"]      = "curr_cxs";
	mss ["total_connections"]     = "total_cxs";
	mss ["connection_structures"] = "cx_structures";
	mss ["cmd_get"]               = "cmd_get";
	mss ["cmd_set"]               = "cmd_set";
	mss ["get_hits"]              = "get_hits";
	mss ["get_misses"]            = "get_misses";
	mss ["evictions"]             = "evictions";
	mss ["bytes_read"]            = "bytes_read";
	mss ["bytes_written"]         = "bytes_written";
	mss ["threads"]               = "threads";

	mc = memcached_create(NULL);
	if (mc == NULL) {
	    cerr << "could not create memcached_st ... don't know what to do ..." << endl;
	    return;
	}

	rc = memcached_server_add (mc, servername.c_str(), port);
	if (rc != MEMCACHED_SUCCESS) {
	    cerr << "memcached_server_add (" << servername << ":" << port << ") failed :"
		 << memcached_strerror(mc, rc)
		 << endl;
	}
    }

    bool MMemcached::measure (string &result) {
	memcached_stat_st *memc_stat = NULL;
	memc_stat = memcached_stat(mc, NULL, &rc);
	map<string, string>::iterator li;
	int i;

	if ((memc_stat == NULL) || ((rc != MEMCACHED_SUCCESS) && (rc != MEMCACHED_SOME_ERRORS))) {
	    cerr << "Failure to communicate with servers : "
		 << memcached_strerror(mc, rc)
		 << endl;
	    if (memc_stat != NULL) {
		memcached_stat_free (mc, memc_stat);
	    }
	    for (li=mss.begin(),i=0 ; li!=mss.end() ; li++,i++) {
		if (i != 0) result += ':';
		result += 'U';
	    }
	    return true;
	}

	for (li=mss.begin(),i=0 ; li!=mss.end() ; li++,i++) {
	    if (i != 0) result += ':';

	    char *value= memcached_stat_get_value(mc, &memc_stat[0], li->first.c_str(), &rc);
	    if ((value == NULL) || (rc != MEMCACHED_SUCCESS)) {
		cerr << "stating memcached " << servername << ":" << port << "(" << li->first << ") failed : "
		     << memcached_strerror(mc, rc)
		     << endl;
		result += 'U';
	    } else {
		result += value;
		free (value);
	    }
	}

	return true;
    }

    MeasurePoint* MMemcached_creator (const string & param) {
	return new MMemcached (param);
    }

    int MMemcached::get_nbpoints (void) {
	return mss.size();
    }

    string MMemcached::get_tagsub (int n) {
	if ((n>=0) && (n<(int)mss.size())) {
	    map<string, string>::iterator li;
	    int i;
	    for (li=mss.begin(),i=0 ; (li!=mss.end()) && (i<n) ; li++,i++);
	    if (li!=mss.end()) {
		return li->second;
	    }
	}
	return "";
    }
    
    string MMemcached::get_next_rras (int i) {
// static char *s[] = {"free", "buffers", "used", "sw_used", "sw_free"};
static const char *s[] =    {"MIN", "MAX", "MAX", "MAX", "MAX", "MIN"};
	if ((i>=0) && (i<6))
	    return s[i];
	else
	    return "";
    }
    
/*
 *  ---- MMySQLGStatus -------------------------------------------------------------------------------------
 */

    MMySQLGStatus::~MMySQLGStatus (void) {
	if (conn != NULL) {
	    mysql_close (conn);
	}
    }
    
    string MMySQLGStatus::maigreconsomne (const string & s) {
	string tosupress = "_-aeiouAEIOU";
//	if (s.size() < 12)
//	    return s;

	string r, rr;
	int i = s.size() - 1, 
	       nbsup = 0,
	       sup = s.size() - 12; // nb chars to supress

	while (i>=0) {
	    if (nbsup < sup) {
		if ((i>0) && (tosupress.find(s[i]) != string::npos)) {
		    nbsup++;
		} else
		    r += s[i];
	    } else
		r += s[i];
	    i--;
	}
cerr << " r_revsup = " << r << endl;
	char t;
	for (i=0 ; ((size_t)i)<r.size()/2 ; i++) {
	    t = r[i];
	    r[i] = r[r.size()-1-i];
	    r[r.size()-1-i] = t;
	}
cerr << " r_sup = " << r << endl;

	rr = r.substr (0, 12);
cerr << " rr = " << rr << endl;
	if (gvar.find(rr) != gvar.end()) {
	    for (i=0 ; i<10 ; i++) {
		if (gvar.find(rr+(char)('0'+i)) == gvar.end()) {
		    rr += (char)('0'+i);
		    break;
		}
	    }
	}
cerr << " rr = " << rr << endl;
	return rr;
    }

    MMySQLGStatus::MMySQLGStatus (const string & param) : MeasureMultiPoint (param) {
	map<string,int> stypes;
	stypes["GAUGE"] = 0;
	stypes["COUNTER"] = 0;
	stypes["DERIVE"] = 0;
	stypes["ABSOLUTE"] = 0;
	string cur_source_type = "DERIVE";

	name = "mysqlgstatus";

	// some default (silly) values
	dbuser = "root";
	dbpass = "";
	dbserver = "localhost";

	size_t p, q;
	q = param.find(',');
	if (q != string::npos) {
	    string connect = param.substr (0,q);

	    p = connect.find(':');
	    if (p != string::npos) {
		if (p > 0) // empty means default
		    dbuser = connect.substr(0, p);
		p++;
	    } else
		p = 0;

	    size_t pp = connect.find ('@', p);
	    if (pp != string::npos) {
		if (pp-p > 0)
		    dbpass = connect.substr(p, pp-p);
		p = pp+1;
	    }

	    string s = connect.substr (p);
	    if (s.size() != 0)
		dbserver = s;

	    q++;
	} else
	    q = 0;

cerr << "dbuser = " << dbuser << endl
     << "dbpass = " << dbpass << endl
     << "dbserver = " << dbserver << endl;

	do {
	    string ident;
	    p = param.find (',', q);
	    if (p != string::npos) {
		ident = param.substr (q, p-q);
		q = p+1;
	    } else {
		ident = param.substr (q);
	    }
	    
	    if (stypes.find(ident) != stypes.end())
		cur_source_type = ident;
	    else {
		gvar[ident] = maigreconsomne (ident);
		source_type[ident] = cur_source_type;
	    }
	} while (p != string::npos);

{   map<string,string>::iterator li;
    int i = 0;
    for (li=gvar.begin() ; li!=gvar.end() ; li++, i++)
	cerr << "gvar [" << li->first << "] = " << li->second << " (" << source_type[li->first] << ")" << endl;
}
cerr << "======" << endl;


	conn = mysql_init(NULL);
	if (conn == NULL) {
	    cerr << "MMySQLGStatus " << name << ":" << dbuser << "@" << dbserver
		 << " could not mysql_init" << endl;
	    return;
	}

	my_bool reconnect = 1;
	if (mysql_options (conn, MYSQL_OPT_RECONNECT, (char *)(&reconnect))) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_options(MYSQL_OPT_RECONNECT) failed. " << mysql_error(conn) << endl;
	}

	if (mysql_real_connect (conn, dbserver.c_str(), dbuser.c_str(), dbpass.c_str(), "", 0, NULL, 0) == NULL) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_real_connect failed. " << mysql_error(conn) << endl;
	}

	// some bug before v5.0.16 clears the reconnect flag ...
	reconnect = 1;
	if (mysql_options (conn, MYSQL_OPT_RECONNECT, (char *)(&reconnect))) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_options(MYSQL_OPT_RECONNECT) failed. " << mysql_error(conn) << endl;
	}
    }

    bool MMySQLGStatus::measure (string &result) {
	int i = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;
	if (mysql_query(conn, "show global status")) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_query(\"show global status\") failed. " << mysql_error(conn) << endl;
	} else {
	    res = mysql_store_result(conn);
	}

	if (res == NULL) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_store_result(\"show global status\") failed. " << mysql_error(conn) << endl;
	    map<string,string>::iterator li;
	    for (li=gvar.begin(),i=0 ; li!=gvar.end() ; li++,i++) {
		if (i != 0) result += ':';
		result += 'U';
	    }
	    return true;
	}

	map<string,string>::iterator mi;
	for (mi=gvar.begin() ; mi!=gvar.end() ; mi++)
	    mi->second = "U";

	int nbrow = 0;
	if (mysql_num_fields(res)<2) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << "not enough columns in result set !" << endl;
	} else {
	    while ((row = mysql_fetch_row(res)) != NULL) {
		nbrow ++;
		if ((mi = gvar.find (row[0])) != gvar.end()) {
		    mi->second = row[1];
		}
	    }
	}

	if (nbrow == 0) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_fetch_row(\"show global status\") failed. " << mysql_error(conn) << endl;
	}

	for (mi=gvar.begin(),i=0 ; mi!=gvar.end() ; mi++,i++) {
	    if (i != 0) result += ':';
	    result += mi->second;
if ("U" == mi->second) {
    cerr << "MMySQLGStatus " << name << ":" << dbuser << "@" << dbserver 
	 << " : variable \"" << mi->first << "\" could not be found in resultset !?" << endl;
}
	}
	return true;
    }

/*
    bool vieille_measure_a_reutiliser (string &result) {
	int i = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;
	if (mysql_query(conn, "show global status")) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_query(\"show global status\") failed. " << mysql_error(conn) << endl;
	} else {
	    res = mysql_store_result(conn);
	}

	if (res == NULL) {
	    cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		 << " mysql_store_result(\"show global status\") failed. " << mysql_error(conn) << endl;
	} else {
	    row = mysql_fetch_row(res);
	    if (row == NULL) {
		cerr << "MMySQLGStatus" << name << ":" << dbuser << "@" << dbserver
		     << " mysql_fetch_row(\"show global status\") failed. " << mysql_error(conn) << endl;
	    }
	}

	if (row == NULL) {
	    map<string,string>::iterator li;
	    for (li=gvar.begin(),i=0 ; li!=gvar.end() ; li++,i++) {
		if (i != 0) result += ':';
		result += 'U';
	    }
	    return true;
	}

	map<string,string>::iterator mi;
	MYSQL_FIELD *field;
	for (mi=gvar.begin() ; mi!=gvar.end() ; mi++)
	    mi->second = "U";
	i = 0;
	while ((field = mysql_fetch_field(res)) != NULL) {
cerr << "  field->name = " << field->name << endl;
	    if ((mi = gvar.find (field->name)) != gvar.end()) {
		mi->second = row[i];
	    }
	    i++;
	}

	for (mi=gvar.begin(),i=0 ; mi!=gvar.end() ; mi++,i++) {
	    if (i != 0) result += ':';
	    result += mi->second;
if ("U" == mi->second) {
    cerr << "MMySQLGStatus " << name << ":" << dbuser << "@" << dbserver 
	 << " : variable " << mi->first << "could not be found in resultset !?" << endl;
}
	}
	return true;
    }
*/

    MeasurePoint* MMySQLGStatus_creator (const string & param) {
	return new MMySQLGStatus (param);
    }

    int MMySQLGStatus::get_nbpoints (void) {
	return gvar.size();
    }

    string MMySQLGStatus::get_tagsub (int n) {
	if ((n>=0) && (n<(int)gvar.size())) {
	    map<string, string>::iterator li;
	    int i;
	    for (li=gvar.begin(),i=0 ; (li!=gvar.end()) && (i<n) ; li++,i++);
	    if (li!=gvar.end()) {
		return li->second;
	    }
	}
	return "";
    }
    
    string MMySQLGStatus::get_source_type (int n) {
	if ((n>=0) && (n<(int)source_type.size())) {
	    map<string, string>::iterator li;
	    int i;
	    for (li=source_type.begin(),i=0 ; (li!=source_type.end()) && (i<n) ; li++,i++);
	    if (li!=source_type.end()) {
		return li->second;
	    }
	}
	return "";
    }
    
    string MMySQLGStatus::get_next_rras (int i) {
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
	mmpcreators["memcached"]    = MMemcached_creator;
	mmpcreators["mysqlgstatus"] = MMySQLGStatus_creator;
	MPfilelen::pcp = pcp;
    }

} // namespace qiconn
