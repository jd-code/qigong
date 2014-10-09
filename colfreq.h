#ifndef COLFREQ_H_INCLUDE
#define COLFREQ_H_INCLUDE

#include <iostream>

namespace qiconn {

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
    
    inline std::ostream& operator<< (std::ostream& cout, CollectFreqDuration const & cfd) {
	return cout << "(" << cfd.interval << " x " << cfd.duration << ")";
    }

} // namespace qiconn

#endif	// COLFREQ_H_INCLUDE
