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
 *  --------------------------------------------------------------------------------------------------------
 */

    bool MPdiskstats::proc_diskstats (const string &disk, int field, string & result) {
	string s;
	ifstream ds("/proc/diskstats");
	size_t l = disk.size();
	while (getstring (ds, s)) {
	    if (s.substr (10, l) == disk) {
// cerr << s << endl ;
		int i;
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
	return proc_diskstats (param, 7, result);
    }

    MeasurePoint* MPdiskstats_creator (const string & param) {
	return new MPdiskstats (param);
    }


    void init_mmpcreators (void) {
	mmpcreators["diskstats"] = MPdiskstats_creator;
    }

} // namespace qiconn
