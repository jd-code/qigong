#ifndef QIGONG_H_INCLUDE
#define QIGONG_H_INCLUDE

#ifdef QIGONG_H_GLOBINST
#define QIGONG_H_SCOPE
#define QIMEASURE_H_GLOBINST
#else
#define QIGONG_H_SCOPE extern
#endif

#include <fstream>
#include <iomanip>

#include <qiconn/qiconn.h>
#include "qicrypt.h"
#include "qicommon.h"
#include "qimeasure.h"

#ifndef QIVERSION
#define QIVERSION "__unversionned at compile time__"
#endif

namespace qiconn {

    using namespace std;

#ifdef QIGONG_H_GLOBINST
    const char * version = "Qigong "QIVERSION;
#else
    extern const char * version;
#endif


    class RecordSet;
    class CollectedConn;

    QIGONG_H_SCOPE map<string, RecordSet*> mrecordsets;
    QIGONG_H_SCOPE multimap<time_t, RecordSet*> schedule;
    
    class RecordSet {
	friend RecordSet * createrecordset (string s, ostream& cerr);
	friend bool suppressrecordset (string &ident, ostream& cerr);
	private:
	    list <MeasurePoint *> lmp;
	    time_t interval;
	    string name;
	    map <CollectedConn *, int> channels;
	    RecordSet () {}
	    ~RecordSet ();
	public:
	    void add_channel  (CollectedConn * pcc);
	    void remove_channel (CollectedConn * pcc);
	    void measure (time_t t);
	    void dump (ostream& cout) const;
	    void dumpdetails (ostream& cout) const;
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
    extern string hostname;
    extern string prompt;
#endif

    class CollectedConn : public CryptConnection
    {
	    int nbp;
	public:
	    virtual const char * gettype (void) { return "CollectedConn"; }
	    void add_subs (RecordSet * prs, bool completereg = true);
	    void remove_subs (RecordSet * prs, bool completereg = true);
	    virtual ~CollectedConn (void);
	    CollectedConn (int fd, struct sockaddr_in const &client_addr, KeyRing* keyring);
	    virtual void firstprompt (void);
	    virtual void lineread (void);
	    map <RecordSet *, int> subs;
	    virtual void poll (void) {};
    };

#ifdef QIGONG_H_GLOBINST
// string theKEY ("JziMb16WKtDCovwKS6ekMBz9uBGsWaKUso/pcHJYRTk= MyDRitse08cmusrP3NDzWw==");
KeyRing keyring;
#else
extern KeyRing keyring;
#endif

    class SocketBinder : public ListeningSocket
    {
	public:
	    virtual ~SocketBinder (void) {}
	    virtual const char * gettype (void) { return "SocketBinder"; }
	    SocketBinder (int fd) : ListeningSocket (fd, "socketbinder") {}
	    virtual SocketConnection* connection_binder (int fd, struct sockaddr_in const &client_addr) {
		return new CollectedConn (fd, client_addr, &keyring);
	    }
	    virtual void poll (void) {}
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
