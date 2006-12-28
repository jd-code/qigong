#include <fstream>
#include <iomanip>

//#define QICONN_H_GLOBINST
//#define QIGONG_H_GLOBINST
//#include "qigong.h"
#include "qiconn.h"
#include "qimeasure.h"

namespace qiconn {
    using namespace std;

    ostream& operator<< (ostream& cout, const MeasurePoint& mp) {
	mp.dump(cout);
	return cout;
    }

/*
 *  ---- MPdiskstats ---------------------------------------------------------------------------------------
 */

    MPdiskstats::MPdiskstats (const string & param) : MeasurePoint (param) {
	name="diskstats";
	size_t p = param.find(' ');
	if (p == string::npos) {
	    diskname = param;
	    field = 7;		// written sectors ?? JDJDJDJD
// cerr << "diskname = <" << diskname << ">   field = <" << field << ">" << endl ;
	    return;
	}
	diskname = param.substr (0, p);
	field = atoi (param.substr (p).c_str());
// cerr << "diskname = <" << diskname << ">   field = <" << field << ">" << endl ;
    }

    bool MPdiskstats::proc_diskstats (string & result) {
	string s;
	ifstream ds("/proc/diskstats");
	size_t l = diskname.size();
	while (getstring (ds, s)) {
	    if (s.substr (10, l) == diskname) {
// cerr << s << endl ;
		size_t i;
		size_t p=10;
		for (i=0 ; i<field ; i++) {
		    p = s.find (' ', p);
		    if (p == string::npos)
			break;
		    p++;
		}
		if (p!=string::npos) {
		    result = s.substr(p, s.find(' ',p) - p);
		    return true;
		    // return (atol (s.substr(p).c_str()));
		} else {
		    return false;
		}
	    }
	    s = "";
	}
	return false;
    }

    bool MPdiskstats::measure (string &result) {
	return proc_diskstats (result);
    }

    MeasurePoint* MPdiskstats_creator (const string & param) {
	return new MPdiskstats (param);
    }


/*
 *  ---- MPnetstats ---------------------------------------------------------------------------------------
 */

    MPnetstats::MPnetstats (const string & param) : MeasurePoint (param) {
	name="netstats";
	size_t p = param.find(' ');
	if (p == string::npos) {
	    intname = param;
	    intname += ':';
	    field = 7;		// written sectors ?? JDJDJDJD
	    return;
	}
	intname = param.substr (0, p);
	intname += ':';
	field = atoi (param.substr (p).c_str());
    }

    bool MPnetstats::proc_net_dev (string & result) {
	string s;
	ifstream ds("/proc/net/dev");
	while (getstring (ds, s)) {
	    size_t p = s.find (intname);
	    if (p != string::npos) {
// cerr << s << endl ;
		p += intname.size();
		p = seekspace (s, p);
		size_t i;
		for (i=0 ; i<field ; i++) {
		    long long dummy;
		    p = getinteger (s, dummy, p);
		    if (p == string::npos)
			break;
		    p = seekspace (s, p);
		    if (p == string::npos)
			break;
		}
		if (p!=string::npos) {
		    result = s.substr(p, s.find(' ',p) - p);
		    return true;
		    // return (atol (s.substr(p).c_str()));
		} else {
		    return false;
		}
	    }
	    s = "";
	}
	return false;
    }

    bool MPnetstats::measure (string &result) {
	return proc_net_dev (result);
    }

    MeasurePoint* MPnetstats_creator (const string & param) {
	return new MPnetstats (param);
    }


    void init_mmpcreators (void) {
	mmpcreators["diskstats"]    = MPdiskstats_creator;
	mmpcreators["netstats"]	    = MPnetstats_creator;
    }

} // namespace qiconn
