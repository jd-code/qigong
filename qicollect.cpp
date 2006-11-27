//  #include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>

#include <rrd.h>

#define QICOLLECT_H_GLOBINST
#define QICONN_H_GLOBINST
#include "qicollect.h"

/*
 *  ------------------- the collecting polling system ----------------------------------------------------
 */

namespace qiconn {
    using namespace std;

    int CollectPool::select_poll (struct timeval *timeout) {
	int r = ConnectionPool::select_poll (timeout);

//	time_t t = time (NULL);
//	map<time_t, RecordSet*>::iterator mi;
//	list <multimap<time_t, RecordSet*>::iterator> lmi;
//	for (mi=schedule.begin() ; (mi!=schedule.end()) && (mi->first < t) ; mi++) {
//	    lmi.push_back(mi);
//	    mi->second->measure(mi->first);
//	}
//	list <multimap<time_t, RecordSet*>::iterator>::iterator li;
//	for (li=lmi.begin() ; li!=lmi.end() ; li++)
//	    schedule.erase(*li);
	return r;
    }

/*
 *  ------------------- the collecting connections -------------------------------------------------------
 */

    CollectingConn::CollectingConn (int fd, struct sockaddr_in const &client_addr) : DummyConnection(fd, client_addr) {
	state = welcome;
	pending = false,
	collecting = false;
	waiting_pcs = NULL;
	waiting_pcs_nextstate = collect;
    }

    bool CollectingConn::assign (CollectionSet * pcc) {
	mpcs[pcc->getkey()] = pcc;
	pcc->state = del_remote_for_create;
	pending = true;
	return true;
    }

    void CollectingConn::lineread (void) {
// cerr << "[" << getname() << "] got[" << bufin << "]" << endl;
	switch (state) {
	    case welcome:
		if (bufin.substr(0, 7) == "qigong[") {
cerr << "[" << getname() << "] ------------------------------>switching to state verify" << endl;
		    state = verify;
		    nbtest = 0;
		    (*out) << "qiging ?" << eos();
		    flush();
		} else {
		    (*out) << eos();
		    flush();
		}
		break;

	    case verify:
		if (bufin.substr(0, 7) == "qigong." ) {
cerr << "[" << getname() << "] ----------------------------->switching to state ready" << endl;
		    state = ready;
		    collecting = true;
		} else {
		    nbtest ++;
		    if (nbtest >= CC__MAXRETRY) {
			// JDJDJDJD faire qqchose pour g�rer time-outs, retry et echecs....
			state = timeout;
cerr << "[" << getname() << "] ----------------------------->switching to state timeout" << endl;
		    }
		    (*out) << "qiging ?" << eos();
		    flush();
		}
		break;

	    case waiting:
		if (bufin.find(wait_string) == 0) {
		    if (waiting_pcs != NULL) {
			waiting_pcs->state = waiting_pcs_nextstate;
			waiting_pcs = NULL;
		    }
cerr << "[" << getname() << "] ----------------------------->switching to state ready" << endl;
		    state = ready;
		} else
		    cerr <<  "[" << getname() << "] waiting for \"" << wait_string << "\" got \"" << bufin << "\". garbaged." << endl;
		break;
		
	    case ready:
	    case timeout:
		break;
	}
    }

    void CollectingConn::poll (void) {
	if (state==ready) {
	    if (pending) {
		MpCS::iterator mi;
		pending = false;
		for (mi=mpcs.begin() ; mi!=mpcs.end() ; mi++) {
		    switch (mi->second->state) {
			case del_remote_for_create:
			    mi->second->delete_remote ();
			    state = waiting;
			    wait_string = "delete:";
			    waiting_pcs = mi->second;
			    waiting_pcs_nextstate = create_remote;
cerr << "[" << getname() << "] ----------------------------->switching to state waiting(" << wait_string << ")" << endl;
			    pending = true;
			    break;
			case create_remote:
			    mi->second->create_remote ();
			    state = waiting;
			    wait_string = "creation done.";
			    waiting_pcs = mi->second;
			    waiting_pcs_nextstate = sub_remote;
cerr << "[" << getname() << "] ----------------------------->switching to state waiting(" << wait_string << ")" << endl;
			    pending = true;
			    break;
			case sub_remote:
			    mi->second->sub_remote ();
			    state = waiting;
			    wait_string = "subscribed";
			    waiting_pcs = mi->second;
cerr << "[" << getname() << "] ----------------------------->switching to state waiting(" << wait_string << ")" << endl;
			    waiting_pcs_nextstate = collect;
			    pending = true;
			    break;
			case collect:
			    break;

			case unmatched:
			case del_remote:
			case unsub_remote:
			    break;
		    }
		    
		    if (pending)
			break;
		}
	    }
	}
    }
    
    CollectingConn::~CollectingConn (void) {
    }
    
/*
 *  ------------------- TaggedMeasuredPoint --------------------------------------------------------------
 */

    string TaggedMeasuredPoint::get_DSdef (time_t heartbeat) {
	stringstream ds;
	ds << "DS:" << tagname << ":" 
	   << "GAUGE" << ":"		    // JDJDJDJD il faudrait rajouter �a dans la conf ???? ou via une d�finition de qqpart
	   << heartbeat << ":"
	   << "U" << ":"
	   << "U"
	;
	return ds.str();
    }

    string TaggedMeasuredPoint::getremote_def (void) {
	string s = fn;
	if (!params.empty())
	    s += '(' + params + ')';
	return s;
    }
    
/*
 *  ------------------- CollectionSet --------------------------------------------------------------------
 */

    string pathname = "/home/chiqong/rrd";

//    char * s_to_malloc (string &s) {
//	char *b = (char *) malloc (s.size()+1);
//	if (b == NULL)
//	    return NULL;
//	strncpy (b, s.c_str(), s.size()+1);
//	return b;
//    }
//    char * s_to_malloc (stringstream &s) {
//	return s_to_malloc (s.c_str());
//    }

    class CharPP {
	private:
	    char ** charpp;
	    int n;
	    bool isgood;

	public:
	    CharPP (string const & s) {
		isgood = false;
		size_t size = s.size();
		size_t p;
		n = 0;
		char *buf = (char *) malloc (size+1);
		if (buf == NULL)
		    return;
		
		list <char *> lp;
		lp.push_back (buf);
		for (p=0 ; p<size ; p++) {
		    if (s[p] == 0)
			lp.push_back (buf + p + 1);
		    buf[p] = s[p];
		}
		n = lp.size() - 1;
		charpp = (char **) malloc ((n+1) * sizeof (char *));
		if (charpp == NULL) {
		    delete (buf);
		    n = 0;
		    return;
		}
		list <char *>::iterator li;
		int i;
		for (li=lp.begin(), i=0 ; (li!=lp.end()) && (i<n) ; li++, i++)
		    charpp[i] = *li;
		charpp[i] = NULL;
		isgood = true;
	    }
	    char ** get_charpp (void) {
		if (isgood)
		    return charpp;
		else
		    return NULL;
	    }
	    size_t size (void) {
		return n;
	    }
	    ~CharPP (void) {
		if (charpp != NULL) {
		    if (n>0)
			delete (charpp[0]);
		    delete (charpp);
		}
	    }
	    ostream& dump (ostream& cout) const {
		int i = 0;
		while (charpp[i] != NULL)
		    cout << "    " << charpp[i++] << endl;
		return cout;
	    }
    };
    ostream & operator<< (ostream& cout, CharPP const & chpp) {
	return chpp.dump (cout);
    }
    
    int CollectionSet::validate_freqs (void) {
	lfreq.sort();
	int nbw = 0;
	base_interval = lfreq.begin()->interval;
	list<CollectFreqDuration>::iterator li;
	for (li=lfreq.begin() ; li!=lfreq.end() ; li++) {
	    long rounded = ((long)(li->interval/base_interval))*base_interval;
	    if (li->interval % base_interval != 0) {
		cerr << "warning : " << "CollectionSet " << metaname << "_" << name << "subsequent entry "
		     << li->interval << "s isn't a multiple of base interval " << base_interval << "s." << endl
		     << "          It will be rounded to " << rounded << " when processed..." << endl;
		nbw++;
	    }
	    if (li->duration % rounded != 0) {
		cerr << "warning : " << "CollectionSet " << metaname << "_" << name << "subsequent entry "
		    << li->duration << "s isn't a multiple of interval " << rounded << "s"
		    << ( (li->interval % base_interval != 0) ? " (rounded)." : "." )  << endl
		    << "           It will be rounded to " << ((long)(1+li->duration/rounded))*li->interval << "s." << endl;
	    }
	}
	return nbw;
    }

    void CollectionSet::buildmissing_rrd (void) {
	string fullname = pathname + '/' + key;
	struct stat buf;
	if (stat (fullname.c_str(), &buf) == 0) {
	    cerr << "error: will not re-create already existing rrd \"" << fullname << "\"." << endl
		 << "         change the collect name in configuration or remove/rename existing rrd" << endl;
// JDJDJDJD marquer inactive
// creer un ficher .def avec chaque rrd ????
	    return;
	}

	time_t heartbeat = 2 * base_interval;

//	const char ** rdd_create_query = (const char **) malloc (sizeof (char *) * nbargs + 1);

	stringstream rcq;

	rcq << "create"		<< eos()
	    << fullname.c_str() << eos()
	    << "--start"	<< eos()
	    << time(NULL)	<< eos()
	    << "--step"		<< eos()
	    << base_interval	<< eos()
	;

	{   list<TaggedMeasuredPoint*>::iterator li;
	    for (li=lptagmp.begin() ; li!=lptagmp.end() ; li++)
		rcq << (*li)->get_DSdef(heartbeat) << eos();
	}

	{   list<CollectFreqDuration>::iterator li;
	    for (li=lfreq.begin() ; li!=lfreq.end() ; li++) {
		rcq << "RRA" << ":"
		    << "LAST" << ":"	// JDJDJDJD this too should be configurable....
		    << "0.5" << ":"	// JDJDJDJD this value is set here almost as a guess (to be explained, please !)	
		    << (long)(li->interval/base_interval) << ":"
		    << (long)(li->duration/li->interval)
		    << eos()
		;
	    }
	}

	CharPP rdd_create_query (rcq.str());
	if (rdd_create_query.get_charpp() == NULL) {
	    cerr << "could not allocate mem for rdd_create_query" << endl;
	    return;
	}

	// cout << "rrd_create (" << endl << rdd_create_query << ")" << endl ;
	
	if (rrd_create (rdd_create_query.size(), rdd_create_query.get_charpp()) != 0) {
	    int e = errno;
	    cerr << "error in rdd_create" << ": " << e << " = " << strerror(e) << endl;
	}
    }

    void CollectionSet::bindcc (CollectingConn *pcc) {
	CollectionSet::pcc = pcc;
    }
    
    int CollectionSet::create_remote (void) {
cerr << "                                                       key=" << key << endl;
	pcc->get_out() << "create " << key << ' ' << (long)base_interval ;
	list<TaggedMeasuredPoint*>::iterator li;
	for (li=lptagmp.begin() ; li!=lptagmp.end() ; li++) {
	    pcc->get_out() << " " << (*li)->getremote_def();
	}
	pcc->get_out() << eos();
	pcc->flush();
	return 0;
    }
    int CollectionSet::delete_remote (void) {
	pcc->get_out() << "delete " << key << eos();
	pcc->flush();
	return 0;
    }
    int CollectionSet::sub_remote (void) {
	pcc->get_out() << "sub " << key << eos();
	pcc->flush();
	return 0;
    }
    int CollectionSet::unsub_remote (void) {
	pcc->get_out() << "unsub " << key << eos();
	pcc->flush();
	return 0;
    }

/*
 *  ------------------- CollectionsConf ------------------------------------------------------------------
 */

    CollectionsConf::~CollectionsConf (void) {
	MpCS::iterator mi;
	for (mi=mpcs.begin() ; mi!=mpcs.end() ; mi++)
	    delete (mi->second);
    }
    bool CollectionsConf::push_back (CollectionSet * pcs) {
	if (pcs==NULL)
	    return false;
	MpCS::iterator mi = mpcs.find(pcs->getkey());
	if (mi != mpcs.end())
	    return false;
	mpcs[pcs->getkey()] = pcs;
	return true;
    }
    bool CollectionsConf::add_host (string name) {
	map<string, int>::iterator mi = hosts_names.find (name);
	if (mi != hosts_names.end())
	    return false;
	hosts_names[name] = 0;
	return true;
    }
    bool CollectionsConf::add_service (string name) {
	map<string, int>::iterator mi = services_names.find (name);
	if (mi != services_names.end())
	    return false;
	services_names[name] = 0;
	return true;
    }
    ostream & CollectionsConf::dump (ostream &cout) const {
	return cout << mpcs;
    }
    

    void CollectionsConf::buildmissing_rrd (void) {
	MpCS::iterator mi;
	for (mi=mpcs.begin() ; mi!=mpcs.end() ; mi++) {
	    mi->second->validate_freqs();
	    mi->second->buildmissing_rrd ();
	}
    }
    
/*
 *  ------------------- readconf -------------------------------------------------------------------------
 */

    class ReadConf {
	private:
	    typedef enum {			// we're waiting for :
				seekdeclare,	// 'host' or 'service'
				seekhostname,	// hostname
				seekhostbegin,	// '{'
				seekcollect,	// 'collect'
				seekcollectname,// collect-name
				seekconnection,	// fqdn(:port)
				seekcolentry,	// next collect entry : () or tagname
				seekrddtabledef,// first entry of rddtabledef
				seekmultiplier,	// the multiplier between the two rddtabledef's elements
				seekrddtable2nd,// second entry of rddtabledef
				seekrddenpar,	// end of rddtabledef
				seektagorend,	// tagname or ending brace
				seektagname,	// tagname
				seekequal,	// equal for tagname affectation
				seekcollectfn,	// collecting function
				seekendparam,	// end of collecting function's parameters
			     } RCState;
	    
	    size_t line;
	    RCState state;

	    // the current metaname
	    string metaname;
	    
	    // the currently in-creation-process TaggedMeasuredPoint
	    string tagmp_tagname,
		   tagmp_fn,
		   tagmp_params;
	    // the currently in-creation CollectionSet
	    string curcs_name,
		   curcs_fqdn;
	    int	   curcs_port;
	    CollectionSet* pcurcs;
	    // the currently built CollectFreqDuration
	    CollectFreqDuration freq;
	    
	    void safeclose (void);
	public:
	    ReadConf (void) {}
	    int readconf (istream& cin, CollectionsConf &conf);
    };
    
    void ReadConf::safeclose (void) {
	if (pcurcs != NULL) {
	    cerr << "(deleting one temporary CollectionSet)" << endl;
	    delete (pcurcs);
	}
    }
    
    int ReadConf::readconf (istream& cin, CollectionsConf &conf) {
	line = 0;
	state = seekdeclare;
	pcurcs = NULL;
	char c;
	while (cin) {
	    string s;
	    long v;
	    size_t p;

	    while (cin.get(c) && (c!=10) && (c!=13)) s += c;
	    if ((c==10) && (cin.peek()==13))
		    cin.get(c);
	    else if ((c==13) && (cin.peek()==10))
		    cin.get(c);
	    line ++;

	    p = seekspace (s);
	    if ((p==string::npos) || (s[p]=='#'))
		continue;

	    string ident;

	    while ((p != string::npos) && (p<s.size())) {
		switch (state) {
		    case seekendparam:
			if (s.find(')') == string::npos) {
			    cerr << "line: " << line << " : could no find matching ending parenthesis on the same line" << endl;
			    safeclose() ; return -1;
			}
			ident = s.substr(p,s.find(')',p)-p);
			tagmp_params = ident;
			if (pcurcs != NULL) {
			    TaggedMeasuredPoint* p = new TaggedMeasuredPoint (tagmp_tagname, tagmp_fn, tagmp_params);
			    if (p == NULL) {
				cerr << "line: " << line << " : could allocate TaggedMeasuredPoint(" << tagmp_tagname << ") : "
				     << strerror(errno) << endl;
				safeclose() ; return -1;
			    }
			    pcurcs->push_back (p);
			} else {
			    cerr << "line: " << line << " : trying to push some TaggedMeasuredPoint when no CollectionSet is defined !" << endl;
			    safeclose() ; return -1;
			}
			// cout << "-----------> param: " << ident << endl;
			p = s.find(')') + 1;
			state = seektagorend;
			break;

		    case seekcollectfn:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for collecting function could not find any suitable ident" << endl;
			    safeclose() ; return -1;
			}
			tagmp_fn = ident;
			// cout << "-----------> function: " << ident << endl;
			if (p!=string::npos && (s[p]=='(')) {
			    p++;
			    state = seekendparam;
			} else {
			    tagmp_params = "";
			    if (pcurcs != NULL) {
				TaggedMeasuredPoint* p = new TaggedMeasuredPoint (tagmp_tagname, tagmp_fn, tagmp_params);
				if (p == NULL) {
				    cerr << "line: " << line << " : could allocate TaggedMeasuredPoint(" << tagmp_tagname << ") : "
					 << strerror(errno) << endl;
				    safeclose() ; return -1;
				}
				pcurcs->push_back (p);
			    } else {
				cerr << "line: " << line << " : trying to push some TaggedMeasuredPoint when no CollectionSet is defined !" << endl;
				safeclose() ; return -1;
			    }
			    state = seektagorend;
			}
			break;

		    case seekequal:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] != '=') {
			    cerr << "line: " << line << " : could not find '=' " << s[p] << "' instead" << endl;
			    safeclose() ; return -1;
			}
			p++;
			state = seekcollectfn;
			break;

		    case seektagorend:
			 seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] == '}') {
			    p++;
			    if (pcurcs == NULL) {
				cerr << "line: " << line << " : trying to end the definition of CollectionSet whith NULL allocated ?" << endl;
				safeclose() ; return -1;
			    }
			    if (pcurcs->validate_freqs() != 0) {
				cerr << "line: " << line << " : error : the warnings above are yet treated as errors." << endl;
				safeclose() ; return -1;
			    }
			    if (!conf.push_back(pcurcs)) {
				cerr << "line: " << line << " : error : could not add CollectionSet(" << curcs_name << "). already registred maybe ?." << endl;
				safeclose() ; return -1;
			    }
			    pcurcs = NULL;
			    metaname = "";
			    state = seekdeclare;
			} else {
			    state = seektagname;
			}
			break;

		    case seektagname:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for tagname could not find any suitable ident" << endl;
			    safeclose() ; return -1;
			}
			if (ident=="collect") {
			    if (pcurcs == NULL) {
				cerr << "line: " << line << " : trying to end the definition of CollectionSet whith NULL allocated ?" << endl;
				safeclose() ; return -1;
			    }
			    if (pcurcs->validate_freqs() != 0) {
				cerr << "line: " << line << " : the warnings above are yet treated as errors." << endl;
				safeclose() ; return -1;
			    }
			    if (!conf.push_back(pcurcs)) {
				cerr << "line: " << line << " : error : could not add CollectionSet(" << curcs_name << "). already registred maybe ?." << endl;
				safeclose() ; return -1;
			    }
			    pcurcs = NULL;
			    state = seekcollectname;
			} else {
			    tagmp_tagname = ident;
			    // cout << "-----------> tagname: " << ident << endl;
			    state = seekequal;
			}
			break;


		    case seekrddenpar:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] != ')') {
			    cerr << "line: " << line << " : could not find ')' " << s[p] << "' instead" << endl;
			    safeclose() ; return -1;
			}
			p++;
			if (pcurcs != NULL) {
			    pcurcs->push_back (freq);
			} else {
			    cerr << "line: " << line << " : trying to push some CollectFreqDuration when no CollectionSet is defined !" << endl;
			    safeclose() ; return -1;
			}
			state = seekcolentry;
			break;

		    case seekrddtable2nd:
			v = 0;
			p = getinteger (s, v, p);
			if (v == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for rddtabledef could not find any suitable value" << endl;
			    cerr << "   arround:" << s.substr(p) << endl;
			    safeclose() ; return -1;
			}
			if (p==string::npos) {
			    cerr << "line: " << line << " : seeking for rddtabledef missing time definition (s,m,h D,W,Y)" << endl;
			    safeclose() ; return -1;
			}
			switch (s[p]) {
			    case 's':			break;
			    case 'm': v*=60;		break;
			    case 'h': v*=60*60;		break;
			    case 'D':
			    case 'd': v*=60*60*24;	break;
			    case 'W':
			    case 'w': v*=60*60*24*7;	break;
			    case 'Y':
			    case 'y': v*=60*60*24*366;	break;
			    default:
			    cerr << "line: " << line << " : seeking for rddtabledef missing time definition (s,m,h D,W,Y)" << endl;
			    safeclose() ; return -1;
			}
			p++;
			freq.duration = v;
			// cout << "-----------> duration: " << v << endl; 
			state = seekrddenpar;
			break;
		    
		    case seekmultiplier:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] != 'x') {
			    cerr << "line: " << line << " : could not find 'x' " << s[p] << "' instead" << endl;
			    safeclose() ; return -1;
			}
			p++;
			state = seekrddtable2nd;
			break;

		    case seekrddtabledef:
			v = 0;
			p = getinteger (s, v, p);
			if (v == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for rddtabledef could not find any suitable value" << endl;
			    cerr << "   arround:" << s.substr(p) << endl;
			    safeclose() ; return -1;
			}
			if (p==string::npos) {
			    cerr << "line: " << line << " : seeking for rddtabledef missing time definition (s,m,h D,W,Y)" << endl;
			    safeclose() ; return -1;
			}
			switch (s[p]) {
			    case 's':			break;
			    case 'm': v*=60;		break;
			    case 'h': v*=60*60;		break;
			    case 'D':
			    case 'd': v*=60*60*24;	break;
			    case 'W':
			    case 'w': v*=60*60*24*7;	break;
			    case 'Y':
			    case 'y': v*=60*60*24*366;	break;
			    default:
			    cerr << "line: " << line << " : seeking for rddtabledef missing time definition (s,m,h D,W,Y)" << endl;
			    safeclose() ; return -1;
			}
			p++;
			freq.interval = v;
			// cout << "-----------> interval: " << v << endl; 
			state = seekmultiplier;
			break;
		    
		    case seekcolentry:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] == '(') {
			    p++;
			    state = seekrddtabledef;
			} else {
			    state = seektagname;
			}
			break;

		    case seekconnection:
			p = getfqdn (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for identifiername could not find any suitable ident" << endl;
			    safeclose() ; return -1;
			}
			curcs_fqdn = ident;
			// cout << "-----------> connection: " << ident << endl;
			if ((p!=string::npos) && (s[p]==':')) { // there's a port attached
			    v = 0;
			    p = getinteger (s, v, p);
			    if (v == 0) {
				cerr << "line: " << line << " : seeking for connection port could not find suitable value" << endl;
				safeclose() ; return -1;
			    }
			    curcs_port = v;
			    if (metaname.empty()) {
				cerr << "line: " << line << " : no host or service currently declared cannot create CollectionSet" << endl;
				safeclose() ; return -1;
			    }
			    pcurcs = new CollectionSet (curcs_name, metaname, curcs_fqdn, curcs_port);
			    if (pcurcs == NULL) {
				cerr << "line: " << line << " : could not allocate CollectionSet(" << curcs_name << ") : "
				     << strerror(errno) << endl;
				safeclose() ; return -1;
			    }
			    // cout << "-----------> port: " << v << endl;
			} else {
			    // cout << "-----------> port: default" << endl; 
			    if (metaname.empty()) {
				cerr << "line: " << line << " : no host or service currently declared cannot create CollectionSet" << endl;
				safeclose() ; return -1;
			    }
			    pcurcs = new CollectionSet (curcs_name, metaname, curcs_fqdn);
			    if (pcurcs == NULL) {
				cerr << "line: " << line << " : could not allocate CollectionSet(" << curcs_name << ") : "
				     << strerror(errno) << endl;
				safeclose() ; return -1;
			    }
			}
			state = seekcolentry;
			break;

		    case seekcollect:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for collect could not find any keyword" << endl;
			    safeclose() ; return -1;
			}
			else if (ident == "collect") {
			    state = seekcollectname;
			} else {
			    cerr << "line: " << line << " : unknown keyword \"" << ident << "\"" << endl ;
			    safeclose() ; return -1;
			}
			break;
		    case seekcollectname:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for collect-name could not find any suitable ident" << endl;
			    safeclose() ; return -1;
			}
			curcs_name = ident;
			// cout << "-----------> collectname: " << ident << endl;
			state = seekconnection;
			break;


		    case seekdeclare:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for declaration could not find any keyword" << endl;
			    safeclose() ; return -1;
			}
			else if (ident == "host") {
			    state = seekhostname;
			} else {
			    cerr << "line: " << line << " : unknown keyword \"" << ident << "\"" << endl ;
			    safeclose() ; return -1;
			}
			break;
		    case seekhostname:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for identifiername could not find any suitable ident" << endl;
			    safeclose() ; return -1;
			}
			metaname = ident;
			conf.add_host (metaname);
			// cout << "-----------> hostname: " << ident << endl;
			state = seekhostbegin;
			break;

		    case seekhostbegin:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] != '{') {
			    cerr << "line: " << line << " : could not find opening brace, found '" << s[p] << "' instead" << endl;
			    safeclose() ; return -1;
			}
			p++;
			state = seekcollect;
			break;
		}
	    }
	}
	if (state != seekdeclare) {
	    cerr << "line : " << line << " premature end of file !" << endl;
	    safeclose() ; return -1;
	}
	return 0;
    }

    ostream& TaggedMeasuredPoint::dump (ostream& cout) const {
	cout << tagname << " = " << fn;
	if (!params.empty())
	    cout << "(" << params << ")";
	return cout;
    }

    ostream& operator<< (ostream& cout, TaggedMeasuredPoint const & tagmp) {
	return tagmp.dump (cout);
    }
    
    ostream& operator<< (ostream& cout, CollectFreqDuration const & cfd) {
	return cout << "(" << cfd.interval << " x " << cfd.duration << ")";
    }
    
    ostream& CollectionSet::dump (ostream& cout) const {
	cout << "    collect " << name << " " << fp.fqdn ;
	if (fp.port != 1264)
	    cout << ":" << fp.port;
	{   list<CollectFreqDuration>::const_iterator li;
	    for (li=lfreq.begin() ; li!=lfreq.end() ; li++)
		cout << " " << *li;
	    cout << endl;
	}
	{   list<TaggedMeasuredPoint*>::const_iterator li;
	    for (li=lptagmp.begin() ; li!=lptagmp.end() ; li++)
		cout << "         " << **li << endl;
	}
	return cout;
    }

    ostream& operator<< (ostream& cout, CollectionSet const& cs) {
	return cs.dump(cout);
    }
    
    ostream& operator<< (ostream& cout, map<string, CollectionSet *> const& l) {
	map<string, CollectionSet *>::const_iterator li;
	for (li=l.begin() ; li!=l.end() ; li++)
	    cout << *(li->second) ;
	return cout;
    }

    ostream& operator<< (ostream& cout, CollectionsConf const &conf) {
	return conf.dump (cout);
    }
    
    /*
     *  ------------------- Configuration : CollectionsConfEngine --------------------------------------------
     */

    void CollectionsConfEngine::startnpoll (ConnectionPool & cp) {
	MpCS::iterator mi;
	map<FQDNPort, CollectingConn *>::iterator mj;

	for (mi=mpcs.begin() ;mi!=mpcs.end() ; mi++) {
	    mj = mpcc.find(mi->second->get_fqdnport());
	    if (mj == mpcc.end()) {
		struct sockaddr_in sin;
		int fd = init_connect(	mi->second->get_fqdnport().fqdn.c_str(),
					mi->second->get_fqdnport().port,
					&sin
				     );

		CollectingConn *pcc = new CollectingConn (fd, sin);
		mpcc[mi->second->get_fqdnport()] = pcc;
		cp.push(pcc);
		mi->second->bindcc (pcc);
		pcc->assign(mi->second);
	    }
	}
    }
    
    /*
     *	------------------------------------------------------------------------------------------------------
     */
    
    // the current running config
    // CollectionsConf runconfig;
    CollectionsConfEngine runconfig;
    
} // namespace qiconn

using namespace std;
using namespace qiconn;

int main (int nb, char ** cmde) {

    string confname ("test.conf");
    
    ifstream fconf (confname.c_str());
    if (!fconf) {
	cerr << "could not open " << confname << " : " << strerror (errno) << endl;
	return -1;
    }
    int nberr;
    ReadConf rc;
    if ((nberr = rc.readconf (fconf, runconfig)) != 0) {
	cerr << "there were errors reading fonf file \"" << confname << "\"" << endl;
	return -1;
    }
    cout << runconfig;

//     return 0;

    
    CollectPool cp;
    
    cp.init_signal ();
    
    int s = server_pool (1264+1);
    // init_connect ("miso.local", 25);
    if (s < 0) {
	cerr << "could not instanciate connection pool, bailing out !" << endl;
	return -1;
    }
    {
	ListeningBinder *ls = new ListeningBinder (s);
	if (ls != NULL) {
	    ls->setname("*:3308");
	    cp.push (ls);
	}
    }
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    runconfig.buildmissing_rrd ();
    runconfig.startnpoll (cp);

if (false) {
    // --------- let's start talking only on log file
    if (close (0) != 0) {
	cerr << "could not close stdin" << strerror (errno) << endl;
	return -1;
    }
    if (freopen ("/var/log/qicollect.log", "a", stdout) == NULL) {
	cerr << "could not open /var/log/qicollect.log : " << strerror (errno) << endl;
	return -1;
    }
    if (freopen ("/var/log/qicollect.log", "a", stderr) == NULL) {
	cerr << "could not open /var/log/qicollect.log : " << strerror (errno) << endl;
	return -1;
    }

    // --------- and fork to the background
    {	pid_t child = fork ();
	int e = errno;
	switch (child) {
	    case -1:
		cerr << "could not fork to background ! " << strerror (e) <<  " -  exiting..." << endl;
		return -1;
	    case 0:
		break;
	    default:
		cerr << "daemon pid=" << child << endl;
		return 0;
	}
    }
}


    cp.select_loop (timeout);
    
    cerr << "on sort bye" << endl;
    
    return 0;
}
