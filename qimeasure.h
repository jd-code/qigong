#ifndef QIMEASURE_H_INCLUDE
#define QIMEASURE_H_INCLUDE

#ifdef QIMEASURE_H_GLOBINST
#define QIMEASURE_H_SCOPE
#else
#define QIMEASURE_H_SCOPE extern
#endif

#include <sys/types.h>	// struct stat
#include <sys/stat.h>	// struct stat
#include <unistd.h>	// struct stat
#ifdef USEMEMCACHED
#include <libmemcached/memcached.h>
#endif
#ifdef USEMYSQL
#include <mysql.h>
#endif

#include <string>
#include <map>

#include "qicommon.h"

namespace qiconn {

    using namespace std;

#ifdef QIMEASURE_H_GLOBINST
    QIMEASURE_H_SCOPE bool debug_mpfilelen_reopen = false;	//!< debug reopening filelen
#else
    QIMEASURE_H_SCOPE bool debug_mpfilelen_reopen;		//!< debug reopening filelen
#endif


    typedef int (*PFNameMatchFunc) (const string &fname, struct stat const & fst);

    int matchallfile (const string &fname, struct stat const & fst);
    int get_most_recent (string dirname, string &flast, PFNameMatchFunc fnmatch = matchallfile);

    class MeasurePoint {
	protected:
	    string param;
	    string name;
	public:
	    virtual ~MeasurePoint (void) {}
	    MeasurePoint (const string & param) { MeasurePoint::param = param; }
	    string getname (void) const {
		return name + '(' + param + ')';
	    }

	    void dump (ostream& cout) const {
		cout << getname();
	    }
	    virtual bool measure (string &result) = 0;			// the measuring function itself
	    virtual int get_nbpoints (void) { return 1; }		// the number of measure reported
	    virtual string get_source_type(void) = 0;			// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) = 0;				// min or U for unknown
	    virtual string get_max(void) = 0;				// min or U for unknown
	    virtual string get_first_rra (void) = 0;			// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) = 0;			// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	    virtual string get_tagsub (int i) { return ""; }		// gives the suffix for appending to the tagname
	    virtual string get_source_type (int i) { return "GAUGE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min (int i) { return "0"; }		// min or U for unknown
	    virtual string get_max (int i) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (int i) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (int i) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)
    };

    class MeasureMultiPoint : public MeasurePoint {
	protected:
	    string param;
	    string name;
	public:
	    virtual ~MeasureMultiPoint (void) {}
	    MeasureMultiPoint (const string & param) : MeasurePoint (param) {};
	    string getname (void) const {
		return name + '(' + param + ')';
	    }

	    void dump (ostream& cout) const {
		cout << getname();
	    }
	    virtual bool measure (string &result) = 0;			// the measuring function itself
	    virtual int get_nbpoints (void) = 0;			// the number of measure reported

	    virtual string get_source_type (void) { return "GAUGE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min (void) { return "0"; }		// min or U for unknown
	    virtual string get_max (void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	    virtual string get_tagsub (int i) = 0;			// gives the suffix for appending to the tagname
	    virtual string get_source_type (int i) = 0;			// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min (int i) = 0;				// min or U for unknown
	    virtual string get_max (int i) = 0;				// max or U for unknown
	    virtual string get_first_rra (int i) = 0;			// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (int i) = 0;			// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)
    };

    ostream& operator<< (ostream& cout, const MeasurePoint& mp);

    typedef MeasurePoint* (*MPCreator) (const string & param); 

    QIMEASURE_H_SCOPE map<string, MPCreator> mmpcreators;

    void init_mmpcreators (ConnectionPool *pcp);

    /*
     *  ----- MPdiskstats --------------------------------------------------------------------------------------
     */

    class MPdiskstats : public MeasurePoint {
	    string diskname;
	    size_t field;
	public:
	    virtual ~MPdiskstats (void) {}
	    MPdiskstats (const string & param);

	    bool proc_diskstats (string & result);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "DERIVE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "0"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)
    };

    /*
     *  ----- MPnetstats ---------------------------------------------------------------------------------------
     */

    class MPnetstats : public MeasurePoint {
	    string intname;
	    size_t field;
	public:
	    virtual ~MPnetstats (void) {}
	    MPnetstats (const string & param);

	    bool proc_net_dev (string & result);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "DERIVE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "0"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)
    };

    /*
     *  ----- MPfilelen ----------------------------------------------------------------------------------------
     */

    class LogCountConn;
    
    class MPfilelen : public MeasurePoint {
	protected:
	    long long oldnl;
	    time_t tlast_nonzero;
	    time_t zltimeout;
	    string fname;
	    int fd;
	    LogCountConn *plcc;
	    static ConnectionPool *pcp;
	    struct stat curstat;
	    virtual void reopen (bool seekend);
	    
	public:
	    virtual ~MPfilelen (void);
	    MPfilelen (const string & param, time_t zltimeout = 0);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "DERIVE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "0"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

    /*
     *  ----- MPlastmatchfilelen -------------------------------------------------------------------------------
     */

    class GetMostRecent {
	protected:
	    virtual int matchentry (const string &fname, struct stat const & fst);
	public:
	    string dirname;
	    virtual ~GetMostRecent () {}
	    GetMostRecent ();
	    GetMostRecent (const string & dirname);
	    bool getlast (string & flast);
    };
    class MatchBeginEnd : public GetMostRecent {
	protected:
	    virtual int matchentry (const string &fname, struct stat const & fst);
	public:
	    string fname_begin, fname_end;
	    MatchBeginEnd () : GetMostRecent() {};
    };


    class MPlastmatchfilelen : public MPfilelen {
	protected:
	    virtual void reopen (bool seekend);
	    int matchbeginendfile (const string &fname, struct stat const & fst);
	    MatchBeginEnd getmostrecent;
	    
	public:
	    virtual ~MPlastmatchfilelen (void);
	    MPlastmatchfilelen (const string & param);

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

    /*
     *  ----- MPloadavg ----------------------------------------------------------------------------------------
     */

    class MPloadavg : public MeasurePoint {
	protected:
	    
	public:
	    virtual ~MPloadavg (void);
	    MPloadavg (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "GAUGE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "0"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

    /*
     *  ----- MPfilevalue --------------------------------------------------------------------------------------
     */

    class MPfilevalue : public MeasurePoint {
	protected:
	    string filemeasure;
	    
	public:
	    virtual ~MPfilevalue (void);
	    MPfilevalue (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "GAUGE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "U"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

    /*
     *  ----- MPmeminfo ----------------------------------------------------------------------------------------
     */

    class MPmeminfo : public MeasureMultiPoint {
	protected:
	    map<string, long long> mss;
	    
	public:
	    virtual ~MPmeminfo (void);
	    MPmeminfo (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual int get_nbpoints (void);

	    virtual string get_tagsub (int i);				// gives the suffix for appending to the tagname
	    virtual string get_source_type (int i) { return "GAUGE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min (int i) { return "0"; }		// min or U for unknown
	    virtual string get_max (int i) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (int i) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (int i);			// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

#ifdef USEMEMCACHED
    /*
     *  ----- MMemcached ---------------------------------------------------------------------------------------
     */

    class MMemcached : public MeasureMultiPoint {
	protected:
	    string source_type;
	    string servername;
	    int port;
	    map<string, string> mss;
	    memcached_st *mc;
	    memcached_return rc;
	    
	public:
	    virtual ~MMemcached (void);
	    MMemcached (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual int get_nbpoints (void);

	    virtual string get_tagsub (int i);				// gives the suffix for appending to the tagname
	    virtual string get_source_type (int i) { return source_type; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min (int i) { return "0"; }		// min or U for unknown
	    virtual string get_max (int i) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (int i) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (int i);			// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };
#endif
#ifdef USEMYSQL
    /*
     *  ----- MMySQLGStatus ------------------------------------------------------------------------------------
     */

    class MMySQLGStatus : public MeasureMultiPoint {
	protected:
	    MYSQL *conn;
	    string dbuser, dbpass, dbserver;
	    map<string,string> gvar;
	    map<string,string> source_type;
	    bool fetch_slave_state;
	    
	public:
	    virtual ~MMySQLGStatus (void);
	    MMySQLGStatus (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual int get_nbpoints (void);

	    virtual string get_tagsub (int i);				// gives the suffix for appending to the tagname
	    virtual string get_source_type (int i);			// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min (int i) { return "0"; }		// min or U for unknown
	    virtual string get_max (int i) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (int i) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (int i);			// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	    string maigreconsomne (const string &s);

	friend void init_mmpcreators (ConnectionPool *pcp);
    };
#endif
    /*
     *  ----- MPfreespace --------------------------------------------------------------------------------------
     */

    class MPfreespace : public MeasurePoint {
	protected:
	    string fname;
	    int lasterr;
	    time_t lasttimeerr;
	    
	public:
	    virtual ~MPfreespace (void);
	    MPfreespace (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "GAUGE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "0"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

    /*
     *  ----- MPconncount --------------------------------------------------------------------------------------
     */

    class MPconncount : public MeasurePoint {
	protected:
	    
	public:
	    virtual ~MPconncount (void);
	    MPconncount (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "GAUGE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "0"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

} // namespace qiconn

#endif // QIMEASURE_H_INCLUDE
