#ifndef TORE_H_INCLUDE
#define TORE_H_INCLUDE

#include <string>
#include <list>

#include "colfreq.h"

namespace qiconn {

    using namespace std;


    //! a fixed-size revolving multi-time-resolution mmap-storage of timed series
    class Tore {
	private:
	    bool usable;	//!< are we usable ?
	    bool defined;	//!< are we defined yet ?
	    int fd;		//!< the file in behind
	    string filename;	//!< the supporting filename where things would end

	    char *mheader;	//!< pointer to the file header fmap

	    long basetime;	//!< base pulsation in seconds

	    size_t headersize;	//!< the size of the header in use

	public:

    static size_t pagesize;

	    Tore (string filename);
	    ~Tore ();

	    // are we usable ?
	    inline operator const void * () const {
		return usable ? this : NULL;
	    }

	    // not(are we usable ?)
	    inline bool operator ! () const {
		return usable ? false : true;
	    }

	    int readheader ();

	    int specify (int basetime, list<CollectFreqDuration> &lfreq, string const &DSdefinition);
    };

}  // namespace qiconn


#endif // TORE_H_INCLUDE
