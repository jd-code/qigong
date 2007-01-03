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

#include <string>
#include <map>

namespace qiconn {

    using namespace std;

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
	    string fname;
	    int fd;
	    LogCountConn *plcc;
	    static ConnectionPool *pcp;
	    struct stat curstat;
	    void reopen (bool seekend);
	    
	public:
	    virtual ~MPfilelen (void);
	    MPfilelen (const string & param);

	    virtual bool measure (string &result);			// the measuring function itself
	    virtual string get_source_type(void) { return "DERIVE"; }	// GAUGE COUNTER DERIVE ABSOLUTE
	    virtual string get_min(void) { return "0"; }		// min or U for unknown
	    virtual string get_max(void) { return "U"; }		// min or U for unknown
	    virtual string get_first_rra (void) { return "LAST"; }	// consolidation function for first rra (AVERAGE MIN MAX or LMAST)
	    virtual string get_next_rras (void) { return "AVERAGE"; }	// consolidation function for subsequent rras (AVERAGE MIN MAX or LMAST)

	friend void init_mmpcreators (ConnectionPool *pcp);
    };

    /*
     *  ----- MPloadavg ----------------------------------------------------------------------------------------
     */

    class LogCountConn;
    
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
     *  ----- MPmeminfo ----------------------------------------------------------------------------------------
     */

    class LogCountConn;
    
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

} // namespace qiconn

#endif // QIMEASURE_H_INCLUDE
