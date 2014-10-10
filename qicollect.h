#ifndef QICOLLECT_H_INCLUDE
#define QICOLLECT_H_INCLUDE

#ifdef QICOLLECT_H_GLOBINST
#define QICOLLECT_H_SCOPE
#define QIMEASURE_H_GLOBINST
#else
#define QICOLLECT_H_SCOPE extern
#endif

#include <fstream>
#include <iomanip>

#include <qiconn/qiconn.h>
#include "qicrypt.h"
#include "qicommon.h"
#include "qimeasure.h"

#include "colfreq.h"

#ifndef QIVERSION
#define QIVERSION "__unversionned at compile time__"
#endif

namespace qiconn {

    using namespace std;

#ifdef QICOLLECT_H_GLOBINST
    const char * version = "Qicollect "QIVERSION;
    bool debug_ccstates = false;
#else
    QICOLLECT_H_SCOPE const char * version;
    QICOLLECT_H_SCOPE bool debug_ccstates;
#endif





    /*
     *  ------------------- Configuration : TaggedMeasuredPoint ----------------------------------------------
     */

    class TaggedMeasuredPoint
    {
	private:
	    string tagname, fn, params;
	    string DSdef;
	    time_t interval;
	public:
	    TaggedMeasuredPoint (string tagname, string fn, string params) {
		TaggedMeasuredPoint::tagname = tagname,
		TaggedMeasuredPoint::fn = fn,
		TaggedMeasuredPoint::params = params;
		interval = 0;
		// cerr << "new TaggedMeasuredPoint(" <<  tagname << ", " << fn << ", " << params << ")" << endl;
	    }
	    ostream& dump (ostream& cout) const;
	    string get_DSdef (time_t heartbeat);
	    string getremote_def (void);
    };
    
    
    /*
     *  ------------------- Configuration : CollectionSet ----------------------------------------------------
     */

    class CollectingConn;
    typedef enum {
	unmatched,
	del_remote,
	del_remote_for_create,
	create_remote,
	sub_remote,
	activate_remote,
	unsub_remote,
	collect
    } CSState;
    
    class CollectionSet
    {
	private:
	    string name,	// collection name
		   metaname;	// host or service meta-name (not connection)
	    int port;		// port for connection
	    FQDNPort fp;	// fqdn name and port for connection
	    string key, fullkey;
	    QiCrKey const* qicrkey;
	    list<TaggedMeasuredPoint*> lptagmp;
	    list<CollectFreqDuration> lfreq;
	    CollectingConn *pcc;
	    time_t base_interval;

	public:
	    CSState state;

	    inline CollectionSet (string serverkey, string name, string metaname, string fqdn, QiCrKey const* qicrkey=NULL, int port=1264) {
		CollectionSet::name = name;
		CollectionSet::metaname = metaname;
		CollectionSet::qicrkey = qicrkey;
		fp = FQDNPort(fqdn, port);
		// CollectionSet::fqdn = fqdn;
		// CollectionSet::port = port;
		key = metaname + '_' + name;
		if (serverkey.size() != 0)
		    fullkey = serverkey + '_' + key;
		else
		    fullkey = key;
		// cerr << "new CollectionSet(" << name << ", " << fqdn << ":" << port << ") = " << fullkey << endl;
		pcc = NULL;
		base_interval = 0;
		state = unmatched;
	    }
	    inline ~CollectionSet (void) {
		list <TaggedMeasuredPoint*>::iterator li;
		for (li=lptagmp.begin() ; li!=lptagmp.end() ; li++)
		    delete (*li);
	    }
	    inline void push_back (TaggedMeasuredPoint* ptagmp) {
		lptagmp.push_back (ptagmp);
	    }
	    inline void push_back (CollectFreqDuration & freq) {
		lfreq.push_back (freq);
	    }
	    ostream& dump (ostream& cout) const;
	    inline const string & getkey (void) const {
		return key;
	    }
	    inline const string & getfullkey (void) const {
		return fullkey;
	    }
	    inline FQDNPort const& get_fqdnport (void) const {
		return fp;
	    }
	    inline const QiCrKey * get_qicrkey(void) const {
		return qicrkey;
	    }

	    int validate_freqs (void);
	    void buildmissing_rrd (bool warnexist = false);
	    void bindcc (CollectingConn *pcc);
	    int create_remote (void);
	    int delete_remote (void);
	    int sub_remote (void);
	    int activate_remote (void);
	    int unsub_remote (void);

	    ostream&  dumpall (ostream& cout);
    };
    
    
    /*
     *  ------------------- Configuration : CollectionsConf --------------------------------------------------
     */

    typedef map<string, CollectionSet *> MpCS;
    
    class CollectionsConf {
	protected:
	    MpCS mpcs;
	public:
	    CollectionsConf () {}
	    ~CollectionsConf (void);
	    bool push_back (CollectionSet * pcs);
	    ostream & dump (ostream &cout) const;

	    void buildmissing_rrd (void);   // JDJDJDJD might move to CollectionsConfEngine
    };
   
    /*
     *  ------------------- Configuration : CollectionsConfEngine --------------------------------------------
     */

    class CollectionsConfEngine : public CollectionsConf {
// JDJDJDJD for temprary convenience  (accessed from bulkrays part)
//	private:
	public:
	    map<FQDNPort, CollectingConn *> mpcc;
	public:
	    CollectionsConfEngine () {}
	    ~CollectionsConfEngine (void) {};
	    void startnpoll (ConnectionPool & cp);
    };

    /*
     *  ------------------- the measuring polling system -----------------------------------------------------
     */

    class CollectPool : public ConnectionPool
    {
	private:
	protected:
	    virtual void treat_signal (void);
	public:
	    CollectPool (void) : ConnectionPool () {}

	    virtual int select_poll (struct timeval *timeout);

    };

    /*
     *  ------------------- the collecting connections -------------------------------------------------------
     */

    class CollectingConn : public CryptConnection
    {
	private:
	    string fqdn;
	    int port;
	    time_t lastattempt;
	    time_t delay_reconnect;
	    typedef enum {
		needtoconnect,
		challenging,
		welcome,
		verify,
		ready,
		waiting,
		timeout
	    } ConnStat;
	    ConnStat state;
#define CC__MAXRETRY 7
	    int nbtest;
	    map<string, CollectionSet *> mpcs;

	    bool pending,
		 is_collecting;

	    string wait_string;
	    CollectionSet * waiting_pcs;
	    CSState waiting_pcs_nextstate;
	    time_t lastlineread;
	    int nbwakeattempt;

// JDJDJDJD
// rough test
map <string,time_t> rrdlastupdate;
map <string,time_t> lastlatency;

	public:
	    virtual ~CollectingConn (void);
	    virtual const char * gettype (void) { return "CollectingConn"; }
	    CollectingConn (string const & fqdn, int port, const QiCrKey* qicrKey);
	    // CollectingConn (int fd, struct sockaddr_in const &client_addr);
	    void firstprompt (void);
	    virtual void lineread (void);
	    ostream& get_out() { return *out; }
	    bool assign (CollectionSet * pcc);
	    virtual void poll (void);
	    virtual void reconnect_hook (void);
	    void failconnect_treat (bool straightfail);

	    ostream& dumpall (ostream& out);
    };


    class ListeningBinder : public ListeningSocket
    {
	public:
	    virtual ~ListeningBinder (void) {}
	    virtual const char * gettype (void) { return "ListeningBinder"; }
	    ListeningBinder (int fd) : ListeningSocket (fd, "socketbinder") {}
	    virtual SocketConnection* connection_binder (int fd, struct sockaddr_in const &client_addr) {
		// return new CollectingConn (fd, client_addr);
		// JDJD WTF was the above ??? never used ?
		return NULL;
	    }
	    virtual void poll (void) {}
    };



    ostream& operator<< (ostream& cout, TaggedMeasuredPoint const & tagmp);
    ostream& operator<< (ostream& cout, CollectionSet const& cs);
    ostream& operator<< (ostream& cout, map<string, CollectionSet *> const& l);
    ostream& operator<< (ostream& cout, CollectionsConf const &conf);
    
} // namespace qiconn

#endif // QICOLLECT_H_INCLUDE
