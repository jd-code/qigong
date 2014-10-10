#ifndef TORE_H_INCLUDE
#define TORE_H_INCLUDE

#include <string>
#include <list>

#include "colfreq.h"

namespace qiconn {

    using namespace std;
////////// JDJDJDJD this should be public, probably
////////// definitions of DS kinds
////////#define DSK_UNKNOWN	13	// unkown DS kind
////////#define DSK_GAUGE	1	// direct value stored
////////#define DSK_COUNTER	2	// a number that can only go high
////////#define DSK_DERIVE	3	// the value stored is the difference from two reports
////////#define DSK_ABSOLUTE	4	// counter that is reset after read


    typedef enum {
	DSK_UNKNOWN=13,		//!< unkown DS kind
	DSK_GAUGE=1,		//!< direct value stored
	DSK_COUNTER=2,		//!< a number that can only go high
	DSK_DERIVE=3,		//!< the value stored is the difference from two reports
	DSK_ABSOLUTE=4,		//!< counter that is reset after read
    } DSkind;

    DSkind stoDSkind (const string & s);
    ostream& operator<< (ostream& cout, DSkind const & kind);

    //! tore's definition of one measure
    class toreDSdef {
	public:
	    string name;	//!< name of the measure, would appear in legends
	    DSkind kind;	//!< nature of the measure
    };


    //! a fixed-size revolving multi-time-resolution mmap-storage of timed series
    class Tore {
	private:
	    bool usable;	//!< are we usable ?
	    bool defined;	//!< are we defined yet ?
	    int fd;		//!< the file in behind
	    string filename;	//!< the supporting filename where things would end

	    char *mheader;	//!< pointer to the file header fmap

	    long basetime;	//!< base pulsation in seconds
	    long nbbanks;	//!< number of banks of datas
	    long nbMPs;		//!< the number of measures in a row - JDJDJDJD not sure MP stands correct here

	    char *startDSdef;	//!< the begining of the DS definitions (header mapped)
	    char *startbankdef;	//!< the begining of the banks definitions (header mapped)

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
	    int mapall ();

	    int specify (int basetime, list<CollectFreqDuration> &lfreq, string const &DSdefinition);
    };

}  // namespace qiconn


#endif // TORE_H_INCLUDE