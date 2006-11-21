#include <fstream>
#include <iomanip>

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
    }

    void CollectingConn::lineread (void) {
    }

    CollectingConn::~CollectingConn (void) {
    }
    
/*
 *  ------------------- readconf -------------------------------------------------------------------------
 */

    int readconf (istream& cin, list<CollectionSet *>& lpcs) {
	char c;
	size_t line=0;
	enum {			// we're waiting for :
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
	     } state = seekdeclare;

	// the currently in-creation-process TaggedMeasuredPoint
	string tagmp_tagname,
	       tagmp_fn,
	       tagmp_params;
	// the currently in-creation CollectionSet
	string curcs_name,
	       curcs_fqdn;
	int	   curcs_port;
	CollectionSet* pcurcs = NULL;
	// the currently built CollectFreqDuration
	CollectFreqDuration freq(-1, -1);
	    
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
			    return -1;
			}
			ident = s.substr(p,s.find(')',p)-p);
			tagmp_params = ident;
			if (pcurcs != NULL) {
			    TaggedMeasuredPoint* p = new TaggedMeasuredPoint (tagmp_tagname, tagmp_fn, tagmp_params);
			    if (p == NULL) {
				cerr << "line: " << line << " : could allocate TaggedMeasuredPoint(" << tagmp_tagname << ") : "
				     << strerror(errno) << endl;
				return -1;
			    }
			    pcurcs->push_back (p);
			} else {
			    cerr << "line: " << line << " : trying to push some TaggedMeasuredPoint when no CollectionSet is defined !" << endl;
			    return -1;
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
			    return -1;
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
				    return -1;
				}
				pcurcs->push_back (p);
			    } else {
				cerr << "line: " << line << " : trying to push some TaggedMeasuredPoint when no CollectionSet is defined !" << endl;
				return -1;
			    }
			    state = seektagname;
			}
			break;

		    case seekequal:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] != '=') {
			    cerr << "line: " << line << " : could not find '=' " << s[p] << "' instead" << endl;
			    return -1;
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
				return -1;
			    }
			    lpcs.push_back(pcurcs);
			    pcurcs = NULL;
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
			    return -1;
			}
			tagmp_tagname = ident;
			// cout << "-----------> tagname: " << ident << endl;
			state = seekequal;
			break;


		    case seekrddenpar:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] != ')') {
			    cerr << "line: " << line << " : could not find ')' " << s[p] << "' instead" << endl;
			    return -1;
			}
			p++;
			if (pcurcs != NULL) {
			    pcurcs->push_back (freq);
			} else {
			    cerr << "line: " << line << " : trying to push some CollectFreqDuration when no CollectionSet is defined !" << endl;
			    return -1;
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
			    return -1;
			}
			if (p==string::npos) {
			    cerr << "line: " << line << " : seeking for rddtabledef missing time definition (s,m,h D,W,Y)" << endl;
			    return -1;
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
			    return -1;
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
			    return -1;
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
			    return -1;
			}
			if (p==string::npos) {
			    cerr << "line: " << line << " : seeking for rddtabledef missing time definition (s,m,h D,W,Y)" << endl;
			    return -1;
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
			    return -1;
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
			    return -1;
			}
			curcs_fqdn = ident;
			// cout << "-----------> connection: " << ident << endl;
			if ((p!=string::npos) && (s[p]==':')) { // there's a port attached
			    v = 0;
			    p = getinteger (s, v, p);
			    if (v == 0) {
				cerr << "line: " << line << " : seeking for connection port could not find suitable value" << endl;
				return -1;
			    }
			    curcs_port = v;
			    pcurcs = new CollectionSet (curcs_name, curcs_fqdn, curcs_port);
			    if (pcurcs == NULL) {
				cerr << "line: " << line << " : could not allocate CollectionSet(" << curcs_name << ") : "
				     << strerror(errno) << endl;
				return -1;
			    }
			    // cout << "-----------> port: " << v << endl;
			} else {
			    // cout << "-----------> port: default" << endl; 
			    pcurcs = new CollectionSet (curcs_name, curcs_fqdn);
			    if (pcurcs == NULL) {
				cerr << "line: " << line << " : could not allocate CollectionSet(" << curcs_name << ") : "
				     << strerror(errno) << endl;
				return -1;
			    }
			}
			state = seekcolentry;
			break;

		    case seekcollect:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for collect could not find any keyword" << endl;
			    return -1;
			}
			else if (ident == "collect") {
			    state = seekcollectname;
			} else {
			    cerr << "line: " << line << " : unknown keyword \"" << ident << "\"" << endl ;
			    return -1;
			}
			break;
		    case seekcollectname:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for collect-name could not find any suitable ident" << endl;
			    return -1;
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
			    return -1;
			}
			else if (ident == "host") {
			    state = seekhostname;
			} else {
			    cerr << "line: " << line << " : unknown keyword \"" << ident << "\"" << endl ;
			    return -1;
			}
			break;
		    case seekhostname:
			p = getidentifier (s, ident, p);
			if (ident.size() == 0) {
			    if (p==string::npos) continue;
			    cerr << "line: " << line << " : seeking for identifiername could not find any suitable ident" << endl;
			    return -1;
			}
			// cout << "-----------> hostname: " << ident << endl;
			state = seekhostbegin;
			break;

		    case seekhostbegin:
			p = seekspace (s, p);
			if (p==string::npos) continue;
			if (s[p] != '{') {
			    cerr << "line: " << line << " : could not find opening brace, found è" << s[p] << "' instead" << endl;
			    return -1;
			}
			p++;
			state = seekcollect;
			break;
		}
	    }
	}
	if (state != seekdeclare) {
	    cerr << "line : " << line << " premature end of file !" << endl;
	    return -1;
	}
	return 0;
    }

    // the current running config
    list<CollectionSet *> runconfig;
    
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
	cout << "    collect " << name << " " << fqdn ;
	if (port != 1264)
	    cout << ":" << port;
	{   list<CollectFreqDuration>::const_iterator li;
	    for (li=lfreq.begin() ; li!=lfreq.end() ; li++)
		cout << " " << *li;
	    cout << endl;
	}
	{   list<TaggedMeasuredPoint*>::const_iterator li;
	    for (li=lptagmp.begin() ; li!=lptagmp.end() ; li++)
		cout << "         " << **li << endl;
	}
	return cout << endl;
    }

    ostream& operator<< (ostream& cout, CollectionSet const& cs) {
	return cs.dump(cout);
    }
    
    ostream& operator<< (ostream& cout, list<CollectionSet *> const& l) {
	list<CollectionSet *>::const_iterator li;
	for (li=l.begin() ; li!=l.end() ; li++)
	    cout << **li << endl;
	return cout;
    }
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
    if ((nberr = readconf (fconf, runconfig)) != 0) {
	cerr << "there were errors reading fonf file \"" << confname << "\"" << endl;
	return -1;
    }
    cout << runconfig << endl;
    return 0;

    
//    if (close (0) != 0) {
//	cerr << "could not close stdin" << strerror (errno) << endl;
//	return -1;
//    }
//    if (freopen ("/var/log/qicollect.log", "a", stdout) == NULL) {
//	cerr << "could not open /var/log/qicollect.log : " << strerror (errno) << endl;
//	return -1;
//    }
//    if (freopen ("/var/log/qicollect.log", "a", stderr) == NULL) {
//	cerr << "could not open /var/log/qicollect.log : " << strerror (errno) << endl;
//	return -1;
//    }
//
//    {	pid_t child = fork ();
//	int e = errno;
//	switch (child) {
//	    case -1:
//		cerr << "could not fork to background ! " << strerror (e) <<  " -  exiting..." << endl;
//		return -1;
//	    case 0:
//		break;
//	    default:
//		cerr << "daemon pid=" << child << endl;
//		return 0;
//	}
//    }


    CollectPool cp;
    
    cp.init_signal ();
    
    int s = server_pool (3308);
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

    cp.select_loop (timeout);
//    init_connect ("www.nasa.gov", 22);
    
    
    return 0;
}

