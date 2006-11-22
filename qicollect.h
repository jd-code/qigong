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
	public:
	    TaggedMeasuredPoint (string tagname, string fn, string params) {
		TaggedMeasuredPoint::tagname = tagname,
		TaggedMeasuredPoint::fn = fn,
		TaggedMeasuredPoint::params = params;
		// cerr << "new TaggedMeasuredPoint(" <<  tagname << ", " << fn << ", " << params << ")" << endl;
	    }
	    ostream& dump (ostream& cout) const;
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
    };
    
    /*
     *  ------------------- Configuration : CollectionSet ----------------------------------------------------
     */

    class CollectionSet
    {
	private:
	    string name,	// collection name
		   fqdn,	// fqdn name for connection
		   metaname;	// host or service meta-name (not connection)
	    int port;		// port for connection
	    string key;
	    list<TaggedMeasuredPoint*> lptagmp;
	    list<CollectFreqDuration> lfreq;
	public:
	    inline CollectionSet (string name, string metaname, string fqdn, int port=1264) {
		CollectionSet::name = name;
		CollectionSet::metaname = metaname;
		CollectionSet::fqdn = fqdn;
		CollectionSet::port = port;
		key = metaname + '_' + name;
		// cerr << "new CollectionSet(" << name << ", " << fqdn << ":" << port << ")" << endl;
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
    };
    
    
    /*
     *  ------------------- Configuration : CollectionsConf --------------------------------------------------
     */

    class CollectionsConf {
	private:
	    map<string, CollectionSet *> mpcs;
	    map<string, int> hosts_names;
	    map<string, int> services_names;
	public:
	    CollectionsConf () {}
	    ~CollectionsConf (void);
	    bool push_back (CollectionSet * pcs);
	    bool add_host (string name);
	    bool add_service (string name);
	    ostream & dump (ostream &cout) const;
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
	public:
	    virtual ~CollectingConn (void);
	    CollectingConn (int fd, struct sockaddr_in const &client_addr);
	    virtual void lineread (void);
    };


    class ListeningBinder : public ListeningSocket
    {
	public:
	    virtual ~ListeningBinder (void) {}
	    ListeningBinder (int fd) : ListeningSocket (fd, "socketbinder") {}
	    virtual DummyConnection* connection_binder (int fd, struct sockaddr_in const &client_addr) {
		return new CollectingConn (fd, client_addr);
	    }
    };



    ostream& operator<< (ostream& cout, TaggedMeasuredPoint const & tagmp);
    ostream& operator<< (ostream& cout, CollectFreqDuration const & cfd);
    ostream& operator<< (ostream& cout, CollectionSet const& cs);
    ostream& operator<< (ostream& cout, map<string, CollectionSet *> const& l);
    ostream& operator<< (ostream& cout, CollectionsConf const &conf);
    
} // namespace qiconn

#endif // QICOLLECT_H_INCLUDE
