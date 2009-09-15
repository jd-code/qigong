#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#define QICONN_H_GLOBINST
#define QIGONG_H_GLOBINST
#include "qigong.h"

namespace qiconn {
    using namespace std;

    void RecordSet::add_channel  (CollectedConn * pcc) {
	channels[pcc]=1;
    }
    void RecordSet::remove_channel (CollectedConn * pcc) {
	channels.erase(channels.find(pcc));
    }
    
    void RecordSet::measure (time_t t) {
	time_t curt = time(NULL);
	schedule.insert(pair<time_t, RecordSet*>(t + interval, this));
	stringstream buf;
	buf << "-->" << name << ' ' << curt;
	list <MeasurePoint *>::iterator li;
	for (li=lmp.begin() ; li!=lmp.end() ; li++) {
	    string b;
	    (*li)->measure(b);
	    buf << ':' << b;
	}
	buf << endl;

	map <CollectedConn *, int>::iterator mi;
	for (mi=channels.begin() ; mi!=channels.end() ; mi++) {
	    *(mi->first->out) << endl << buf.str();
	    mi->first->flush();
	}
    }
    void RecordSet::dump (ostream& cout) const {
	cout << "RecordSet: \"" << name << "\":" << endl;
	cout << "    RefreshInterval: " << interval << "s" << endl;
	cout << "    MeasurePoint:" << endl;
	list <MeasurePoint *>::const_iterator li;
	for (li=lmp.begin() ; li!=lmp.end() ; li++) {
	    cout << "        " << **li << endl;
	}
    }

    ostream& operator<< (ostream& cout, RecordSet const& rs) {
	rs.dump (cout);
	return cout;
    }

    RecordSet * createrecordset (string s, ostream& cerr) {
	size_t p, q;
	string name;

	p = getidentifier (s, name, 0);
	if (name.size() == 0) {
	    cerr << "error: createrecordset: name is zero bytes long." << endl;
	    return NULL;
	}

	if (p == string::npos) {
	    cerr << "error: createrecordset: missing interval field." << endl;
	    return NULL;
	}

	if (mrecordsets.find (name) != mrecordsets.end()) {
	    cerr << "error: createrecordset: \"" << name << "\" is already registered as a RecordSet." << endl;
	    return NULL;
	}

	time_t interval = 0;
	p = getinteger (s, interval, p);
	
	if (interval == 0) {
	    cerr << "error: createrecordset: null interval given ???" << endl;
	    return NULL;
	}

	RecordSet *prs = new RecordSet;
	if (prs == NULL) {
	    cerr << "error: createrecordset: could not allocate RecordSet..." << endl;
	    return NULL;
	}
	prs->name = name;
	prs->interval = interval;
	
	string ident, param;
	while (p != string::npos) {
	    p = getidentifier (s, ident, p);
	    if (ident.size() == 0) {
		if (p != string::npos)
		    cerr << "error: createrecordset: measure-point identifier is 0 bytes long." << endl;
		break;
	    }
	    if (s[p] == '(') {
		p++;
		q = s.find (')', p);
		if (q == string::npos) {
		    cerr << "error: createrecordset: unmatched parenthesis in param." << endl;
		    p = q;
		    continue;
		}
		param = s.substr(p, q-p);
		p = q+1;
	    } else
		param = "";

	    map<string, MPCreator>::iterator mi = mmpcreators.find (ident);
	    if (mi==mmpcreators.end()) {
		cerr << "error: createrecordset: unknown ident : \"" << ident << "\" ..." << endl;
	    } else {
		// cerr << "good: (" << ident << ")[" << param << "]" << endl;
		MPCreator mpc = mi->second;
		MeasurePoint *pmp = (*mpc) (param);
		if (pmp == NULL)
		    cerr << "error: createrecordset: could not allocate MeasurePoint [" << ident << ", " << param << "] ..." << endl;
		else
		    prs->lmp.push_back(pmp);
	    }
	}
	if (prs->lmp.empty()) {
	    cerr << "error: createrecordset: no MeasurePoint could be set, discarding the \"" << name << "\" RecordSet" << endl ;
	    delete (prs);
	    return NULL;
	}

	mrecordsets[name] = prs;

	//	prs->measure(time (NULL));
	//	cerr << *prs << endl;
	
	return prs;
    }

    RecordSet::~RecordSet (void) {
	list <MeasurePoint *>::iterator li;
	for (li=lmp.begin() ; li!=lmp.end() ; li++)
	    delete (* li);
    }

    bool suppressrecordset (string &ident, ostream& cerr) {
	map<string, RecordSet*>::iterator mi = mrecordsets.find(ident);
	if (mi==mrecordsets.end()) {
	    cerr << "unknown recordset \"" << ident << "\"" << endl;
	    return false;
	}
	RecordSet* prs = mi->second;
	multimap<time_t, RecordSet*>::iterator mmi;
	list <multimap<time_t, RecordSet*>::iterator> lmmi;
	for (mmi=schedule.begin() ; mmi!=schedule.end() ; mmi++) {
	    if (mmi->second == prs)
		lmmi.push_back(mmi);
	}
	list <multimap<time_t, RecordSet*>::iterator>::iterator li;
	cerr << "deleting scheduled entries" << endl;
	for (li=lmmi.begin() ; li!=lmmi.end() ; li++)
	    schedule.erase(*li);

	cerr << "deleting recordset entries" << endl;
	mrecordsets.erase(mi);

	{   cerr << "unsubscribing channels" << endl;
	    map <CollectedConn *, int>::iterator mi;
	    for (mi=prs->channels.begin() ; mi!=prs->channels.end() ; mi++)
		mi->first->remove_subs (prs);
	}
	
	cerr << "deleting recordset object" << endl;
	delete(prs);
	return true;
    }

    bool activaterecordset (string &ident, ostream& cerr) {
	map<string, RecordSet*>::iterator mi = mrecordsets.find(ident);
	if (mi==mrecordsets.end()) {
	    cerr << "unknown recordset \"" << ident << "\"" << endl;
	    return false;
	}
	RecordSet* prs = mi->second;
	prs->measure(time (NULL));
	return true;
    }


/*
 *  ------------------- the collected connections --------------------------------------------------------
 */


    CollectedConn::CollectedConn (int fd, struct sockaddr_in const &client_addr) : DummyConnection(fd, client_addr) {
	nbp = 0;
	char buf[256];
	if (gethostname (buf, 256) != 0)
	    cerr << "could not get hostname : " << strerror (errno) << endl;
	else
	    hostname = buf;
	(*out) << version << " - host:" << hostname << endl << endl;
	prompt = "qigong[" + hostname + "] : ";
	(*out) << prompt << nbp++ << " : " << eos();
	flush();
    }
    CollectedConn::~CollectedConn (void) {
	map <RecordSet *, int>::iterator mi;
	for (mi=subs.begin() ; mi!=subs.end() ; mi++)
	    mi->first->remove_channel (this);
    }

    void CollectedConn::add_subs  (RecordSet * prs, bool completereg /* = true */) {
	subs[prs]=1;
	if (completereg) prs->add_channel (this);
    }
    
    void CollectedConn::remove_subs  (RecordSet * prs, bool completereg /* = true */) {
	subs.erase(subs.find(prs));
	if (completereg) prs->remove_channel (this);
    }
    
    void CollectedConn::lineread (void) {
	string command;
	size_t p;
	//(*out) << "{" << bufin << "}" << endl;
	p = getidentifier (bufin, command);
	if (command.size() == 0) {
	    // (*out) << "tell-me that again dude ?" << endl;

	} else if (command=="help") {	// -------------------------------------------------------------------------------------------------

	    (*out)  << "  help    this help" << endl
		    << "  create RecordSetIdent Interval MeasurePointName(Params ) [ MP(params) ... ]" << endl
		    << "  list" << endl
		    << "  delete RecordSetIdent" << endl
		    << "  activate RecordSetIdent" << endl
		    << "  subscribe|sub RecordSetIdent" << endl
		    << "  unsubscribe|unsub RecordSetIdent" << endl
		    << "  shutdown" << endl
		    << "  quit" << endl
		    << endl;
	    
	} else if (command=="quit") { // -----------------------------------------------------------------------------------------------
	    
	    (*out) << "ending connection" << endl;
	    schedule_for_destruction ();
	    
	} else if (command=="shutdown") { // -----------------------------------------------------------------------------------------------
	    
	    (*out) << "entering shutdown" << endl;
	    if (cp != NULL)
		cp->tikkle();
	    
	} else if (command=="create") {	// -------------------------------------------------------------------------------------------------

	    RecordSet *prs = createrecordset (bufin.substr(p), *out);
	    if (prs == NULL)
		(*out) << "creation failed" << endl;
	    else
		(*out) << *prs << endl << "creation done." << endl;
	    
	} else if (command=="list") {	// -------------------------------------------------------------------------------------------------

	    map<string, RecordSet*>::iterator mi;
	    (*out) << endl;
	    for (mi=mrecordsets.begin() ; mi!=mrecordsets.end() ; mi++)
		(*out) << *(mi->second) << endl;

	} else if (command=="delete") {	// -------------------------------------------------------------------------------------------------

	    string ident;
	    int n=0;
	    while (p != string::npos) {
		p = getidentifier (bufin, ident, p);
		if (ident.empty()) {
		    (*out) << "error: missing identifier" << endl;
		    break;
		} else {
		    if (suppressrecordset(ident, *out))
			n++;
		    else
			(*out) << "deletion of \"" << ident << "\"failed." << endl ;
		}
	    }
	    (*out) << "delete: " << n << " deletion" << ((n!=1) ? "s" : "") << " performed." << endl;

	} else if (command=="activate") {   // ---------------------------------------------------------------------------------------------

	    string ident;
	    int n=0;
	    while (p != string::npos) {
		p = getidentifier (bufin, ident, p);
		if (ident.empty()) {
		    (*out) << "error: missing identifier" << endl;
		    break;
		} else {
		    if (activaterecordset(ident, *out)) {
			(*out) << "RecordSet: \"" << ident << "\" activated." << endl;
			n++;
		    } else
			(*out) << "activation of \"" << ident << "\"failed." << endl ;
		}
	    }
	    (*out) << "activate: " << n << " activation" << ((n!=1) ? "s" : "") << " performed." << endl;

	} else if ((command=="subscribe") || (command=="sub")) {  // -----------------------------------------------------------------------

	    string ident;
	    int n=0;
	    while (p != string::npos) {
		p = getidentifier (bufin, ident, p);
		if (ident.empty()) {
		    (*out) << "error: missing identifier" << endl;
		    break;
		} else {
		    map<string, RecordSet*>::iterator mi = mrecordsets.find (ident);
		    if (mi == mrecordsets.end()) {
			(*out) << "RecordSet: \"" << ident << "\" doesn't exist." << endl;
		    } else {
			add_subs (mi->second);
			(*out) << "subscribed to  \"" << ident << "\"." << endl ;
			n++;
		    }
		}
	    }
	    (*out) << "subscribe: " << n << " subsription" << ((n!=1) ? "s" : "") << " performed." << endl;

	} else if ((command=="unsubscribe") || (command=="unsub")){ // ---------------------------------------------------------------------

	    string ident;
	    int n=0;
	    while (p != string::npos) {
		p = getidentifier (bufin, ident, p);
		if (ident.empty()) {
		    (*out) << "error: missing identifier" << endl;
		    break;
		} else {
		    map<string, RecordSet*>::iterator mi = mrecordsets.find (ident);
		    if (mi == mrecordsets.end()) {
			(*out) << "RecordSet: \"" << ident << "\" doesn't exist." << endl;
		    } else {
			remove_subs (mi->second);
			(*out) << "unsubscribed to  \"" << ident << "\"." << endl ;
			n++;
		    }
		}
	    }
	    (*out) << "unsubscribe: " << n << " unsubsription" << ((n!=1) ? "s" : "") << " performed." << endl;

	} else if (command=="qiging") { // ---------------------------------------------------------------------
	    (*out) << eos() << "qigong." << eos() ;
	    prompt = "";

	} else {    // ---------------------------------------------------------------------------------------------------------------------

	    (*out) << "unknown command : \"" << command << "\"" << endl;
	}
	if (!prompt.empty())
	    (*out) << prompt << nbp++ << " : " << eos();
	flush();
    }

/*
 *  ------------------- the measuring polling system -----------------------------------------------------
 */

    int MeasurePool::select_poll (struct timeval *timeout) {
	int r = ConnectionPool::select_poll (timeout);
	time_t t = time (NULL);
	map<time_t, RecordSet*>::iterator mi;
	list <multimap<time_t, RecordSet*>::iterator> lmi;
	for (mi=schedule.begin() ; (mi!=schedule.end()) && (mi->first < t) ; mi++) {
	    lmi.push_back(mi);
	    mi->second->measure(mi->first);
	}
	list <multimap<time_t, RecordSet*>::iterator>::iterator li;
	for (li=lmi.begin() ; li!=lmi.end() ; li++)
	    schedule.erase(*li);
	return r;
    }

} // namespace qiconn

using namespace std;
using namespace qiconn;

void param_match (const char * param, const char * match, bool &flag) {
    size_t l = strlen (match);
    if (strncmp (param, match, l) == 0)
	flag = true;
}   

void param_match (string param, string match, string &s) {
    match += '=';
    if (param.find(match) == 0)
	s = param.substr (match.size());
}

static SyslogCerrHook cerr_hook;

int main (int nb, char ** cmde) {

    int port = QICONNPORT;
    bool dofork = true;

    string logfile = "/var/log/qigong.log",
	   pidfile = "/var/run/qigong.pid";
    
    {	int i;
	for (i=1 ; i<nb ; i++) {
	    if ((strncmp (cmde[i], "-port", 5) == 0) && (i+1 < nb)) {
		port = atoi (cmde[i+1]);
		i++;
	    }
	    param_match (cmde[i], "-debugresolver",	debug_resolver);
	    param_match (cmde[i], "-debugtransmit",	debug_transmit);
	    param_match (cmde[i], "-debugout",		debug_dummyout);
	    param_match (cmde[i], "-debuginput",	debug_dummyin);
	    param_match (cmde[i], "-debuglineread",	debug_lineread);
	    param_match (cmde[i], "-pidfile",		pidfile);
	    param_match (cmde[i], "-logfile",		logfile);

	    if (strncmp (cmde[i], "-nofork", 7) == 0) {
		dofork = false;
	    }
	    if (strncmp (cmde[i], "--help", 6) == 0) {
		cout << "usage : " << cmde[0] << " [-port N] [-nofork] [-pidfile=fname] [-logfile=fname] [--help]" << endl
		     << "                         [-debugresolver] [-debugtransmit] [-debugout] [-debuginput] [-debuglineread]" << endl
		;
		return 0;
	    }
	}
    }

    if (dofork) {
	cerr_hook.hook (cerr, "qigong", LOG_NDELAY|LOG_NOWAIT|LOG_PID, LOG_DAEMON, LOG_ERR);

	// --------- let's start talking only on log file

	if (close (0) != 0) {
	    cerr << "could not close stdin" << strerror (errno) << endl;
	    return -1;
	}
	if (freopen (logfile.c_str(), "a", stdout) == NULL) {
	    cerr << "could not open " << logfile << " : " << strerror (errno) << endl;
	    return -1;
	}
	if (freopen (logfile.c_str(), "a", stderr) == NULL) {
	    cerr << "could not open " << logfile << " : " << strerror (errno) << endl;
	    return -1;
	}
    }

    int s = server_pool (port);
    // init_connect ("miso.local", 25);
    if (s < 0) {
	cerr << "could not instanciate connection pool, bailing out !" << endl;
	return -1;
    }
    SocketBinder *ls = new SocketBinder (s);
    if (ls == NULL) {
	cerr << "could not instanciate SocketBinder, bailing out !" << endl;
	return -1;
    }

    {	stringstream name;
	name << "*:" << port;
	ls->setname(name.str());
    }

    if (dofork) {
	ofstream pidf(pidfile.c_str());
	if (!pidf) {
	    int e = errno;
	    cerr << "could not open pif-file : " << pidfile << " : " << strerror(e) << endl;
	    return -1;
	}
	
	// --------- and fork to the background
	pid_t child = fork ();
	int e = errno;
	switch (child) {
	    case -1:
		cerr << "could not fork to background ! " << strerror (e) <<  " -  exiting..." << endl;
		return -1;
	    case 0:
		break;
	    default:
		cerr << "daemon pid=" << child << endl;
		pidf << child << endl;
		return 0;
	}
	if (setsid () == -1) {
	    cerr << "setsid failed, certainly not a big deal ! (ignored) " << strerror (errno) << endl;
	}

    }

    MeasurePool cp;

    init_mmpcreators(&cp);

if (false) {
    createrecordset ("allo 12 method(38) diskstats(sda) load() trcumuch(1 2 3   ", cerr);
    createrecordset ("allo 1 method(38) diskstats(sda) load() diskstats", cerr);
    createrecordset ("alli 5 method(38) diskstats(hda) load()", cerr);
    createrecordset ("mexico 5 method(38) diskstats(hda) load()", cerr);
}
    
    cp.init_signal ();
    
    cp.push (ls);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    cp.select_loop (timeout);

    cerr << "terminating" << endl;
    
    return 0;
}
