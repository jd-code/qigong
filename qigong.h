
#ifndef QIGONG_H_INCLUDE
#define QIGONG_H_INCLUDE

#ifdef QIGONG_H_GLOBINST
#define SCOPE
#else
#define SCOPE external
#endif

#include <fstream>
#include <iomanip>

#include "qiconn.h"

namespace qiconn {

    using namespace std;

#ifdef QIGONG_H_GLOBINST
    char * version = "Qigong v0.5 - $Id$";
#else
    external char * version;
#endif

    class MeasurePoint {
	protected:
	    string param;
	    string name;
	public:
	    virtual ~MeasurePoint (void) {}
	    MeasurePoint (const string & param) { MeasurePoint::param = param; }
	    virtual bool measure (string &result) = NULL;
	    string getname (void) const {
		return name + '(' + param + ')';
	    }

	    void dump (ostream& cout) const {
		cout << getname();
	    }
    };

    ostream& operator<< (ostream& cout, const MeasurePoint& mp);

    typedef MeasurePoint* (*MPCreator) (const string & param); 

    SCOPE map<string, MPCreator> mmpcreators;

    /*
     *  --------------------------------------------------------------------------------------------------------
     */

    class MPdiskstats : public MeasurePoint {
	public:
	    virtual ~MPdiskstats (void) {}
	    MPdiskstats (const string & param) : MeasurePoint (param) { name="diskstats"; }

	    bool proc_diskstats (const string &disk, int field, string & result);

	    virtual bool measure (string &result);
    };

    MeasurePoint* MPdiskstats_creator (const string & param);

    class RecordSet;
    class CollectedConn;

    SCOPE map<string, RecordSet*> mrecordsets;
    SCOPE multimap<time_t, RecordSet*> schedule;
    
    class RecordSet {
	friend RecordSet * createrecordset (string s, ostream& cerr);
	friend bool suppressrecordset (string &ident, ostream& cerr);
	private:
	    list <MeasurePoint *> lmp;
	    time_t interval;
	    string name;
	    map <CollectedConn *, int> channels;
	    RecordSet () {}
	    ~RecordSet () {}
	public:
	    void add_channel  (CollectedConn * pcc);
	    void remove_channel (CollectedConn * pcc);
	    void measure (time_t t);
	    void dump (ostream& cout) const;
    };

    ostream& operator<< (ostream& cout, RecordSet const& rs);

    RecordSet * createrecordset (string s, ostream& cerr);

    bool suppressrecordset (string &ident, ostream& cerr);

    bool activaterecordset (string &ident, ostream& cerr);

    /*
     *  create 60s newmexico diskstat(sda) diskstat(sdb) load mem
     *  subscribe newmexico
     *
     */




    /*
     *  ------------------- the collected connections --------------------------------------------------------
     */

#ifdef QIGONG_H_GLOBINST
	     string hostname = "unknown_host";
	     string prompt = "qigong[unknown_host] : ";
#else
    external string hostname;
    external string prompt;
#endif

    class CollectedConn : public DummyConnection
    {
	public:
	    void add_subs (RecordSet * prs, bool completereg = true);
	    void remove_subs (RecordSet * prs, bool completereg = true);
	    virtual ~CollectedConn (void);
	    CollectedConn (int fd, struct sockaddr_in const &client_addr);
	    virtual void lineread (void);
	    map <RecordSet *, int> subs;
    };

    class SocketBinder : public ListeningSocket
    {
	public:
	    virtual ~SocketBinder (void) {}
	    SocketBinder (int fd) : ListeningSocket (fd, "socketbinder") {}
	    virtual DummyConnection* connection_binder (int fd, struct sockaddr_in const &client_addr) {
		return new CollectedConn (fd, client_addr);
	    }
    };

    /*
     *  ------------------- the measuring polling system -----------------------------------------------------
     */

    class MeasurePool : public ConnectionPool
    {
	private:
	public:
	    MeasurePool (void) : ConnectionPool () {}

	    virtual int select_poll (struct timeval *timeout);
    };

    /*
     *  ------------------------------------------------------------------------------------------------------
     */


    void init_mmpcreators (void);

}   // namespace qiconn

#endif // QIGONG_H_INCLUDE
