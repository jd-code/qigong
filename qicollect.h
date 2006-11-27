#ifndef QICOLLECT_H_INCLUDE
#define QICOLLECT_H_INCLUDE

#ifdef QICOLLECT_H_GLOBINST
#define SCOPE
#else
#define SCOPE external
#endif

#include <fstream>
#include <iomanip>

#include "qiconn.h"

namespace qiconn {

    using namespace std;

#ifdef QICOLLECT_H_GLOBINST
    char * version = "Qicollect v0.5 - $Id$";
#else
    external char * version;
#endif





    /*
     *  ------------------- Configuration : TaggedMeasuredPoint ----------------------------------------------
     */

    class TaggedMeasuredPoint
    {
	private:
	    string tagname, fn, params;
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
     *  ------------------- Configuration : CollectFreqDuration ----------------------------------------------
     */

    class CollectFreqDuration
    {
	public: 
	    long interval, duration;
	    inline CollectFreqDuration (long interval = -1, long duration = -1) {
		CollectFreqDuration::interval = interval;
		CollectFreqDuration::duration = duration;
		// cerr << "new CollectFreqDuration (" << interval << ", " << duration << ")" << endl;
	    }
	    bool operator< (CollectFreqDuration const & cfd) const {
		return interval < cfd.interval;
	    }
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
	    string key;
	    list<TaggedMeasuredPoint*> lptagmp;
	    list<CollectFreqDuration> lfreq;
	    CollectingConn *pcc;
	    time_t base_interval;

	public:
	    CSState state;

	    inline CollectionSet (string name, string metaname, string fqdn, int port=1264) {
		CollectionSet::name = name;
		CollectionSet::metaname = metaname;
		fp = FQDNPort(fqdn, port);
		// CollectionSet::fqdn = fqdn;
		// CollectionSet::port = port;
		key = metaname + '_' + name;
		// cerr << "new CollectionSet(" << name << ", " << fqdn << ":" << port << ")" << endl;
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
	    inline FQDNPort const& get_fqdnport (void) const {
		return fp;
	    }

	    int validate_freqs (void);
	    void buildmissing_rrd (void);
	    void bindcc (CollectingConn *pcc);
	    int create_remote (void);
	    int delete_remote (void);
	    int sub_remote (void);
	    int unsub_remote (void);
    };
    
    
    /*
     *  ------------------- Configuration : CollectionsConf --------------------------------------------------
     */

    typedef map<string, CollectionSet *> MpCS;
    
    class CollectionsConf {
	protected:
	    MpCS mpcs;
	    map<string, int> hosts_names;
	    map<string, int> services_names;
	public:
	    CollectionsConf () {}
	    ~CollectionsConf (void);
	    bool push_back (CollectionSet * pcs);
	    bool add_host (string name);
	    bool add_service (string name);
	    ostream & dump (ostream &cout) const;

	    void buildmissing_rrd (void);   // JDJDJDJD might move to CollectionsConfEngine
    };
   
    /*
     *  ------------------- Configuration : CollectionsConfEngine --------------------------------------------
     */

    class CollectionsConfEngine : public CollectionsConf {
	private:
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
	public:
	    CollectPool (void) : ConnectionPool () {}

	    virtual int select_poll (struct timeval *timeout);
    };

    /*
     *  ------------------- the collecting connections -------------------------------------------------------
     */

    class CollectingConn : public DummyConnection
    {
	private:
	    typedef enum {
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
		 collecting;

	    string wait_string;
	    CollectionSet * waiting_pcs;
	    CSState waiting_pcs_nextstate;

	public:
	    virtual ~CollectingConn (void);
	    CollectingConn (int fd, struct sockaddr_in const &client_addr);
	    virtual void lineread (void);
	    ostream& get_out() { return *out; }
	    bool assign (CollectionSet * pcc);
	    virtual void poll (void);
    };


    class ListeningBinder : public ListeningSocket
    {
	public:
	    virtual ~ListeningBinder (void) {}
	    ListeningBinder (int fd) : ListeningSocket (fd, "socketbinder") {}
	    virtual DummyConnection* connection_binder (int fd, struct sockaddr_in const &client_addr) {
		return new CollectingConn (fd, client_addr);
	    }
	    virtual void poll (void) {}
    };



    ostream& operator<< (ostream& cout, TaggedMeasuredPoint const & tagmp);
    ostream& operator<< (ostream& cout, CollectFreqDuration const & cfd);
    ostream& operator<< (ostream& cout, CollectionSet const& cs);
    ostream& operator<< (ostream& cout, map<string, CollectionSet *> const& l);
    ostream& operator<< (ostream& cout, CollectionsConf const &conf);
    
} // namespace qiconn

#endif // QICOLLECT_H_INCLUDE
