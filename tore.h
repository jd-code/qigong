#ifndef TORE_H_INCLUDE
#define TORE_H_INCLUDE

#ifdef TORE_H_GLOBINST
#define TORE_H_SCOPE
#define QIMEASURE_H_GLOBINST
#else
#define TORE_H_SCOPE extern
#endif


#include <string>
#include <list>

#include <stdint.h>

#include "colfreq.h"

namespace qiconn {

#define TORE_TIME_UNKNOWN   314	//!< timestamp for an unknown date or never

    using namespace std;

#ifdef TORE_H_GLOBINST
    bool debug_tore = true;
#else
    TORE_H_SCOPE bool debug_tore;
#endif

    //! nature of a measure point
    typedef enum {
	DSK_UNKNOWN	= 13,	//!< unkown DS kind
	DSK_GAUGE	= 1,	//!< direct value stored
	DSK_COUNTER	= 2,	//!< a number that can only go high
	DSK_DERIVE	= 3,	//!< the value stored is the difference from two reports
	DSK_ABSOLUTE	= 4,	//!< counter that is reset after read
    } DSkind;

    DSkind stoDSkind (const string & s);
    ostream& operator<< (ostream& cout, DSkind const & kind);


    //! tore's definition of one measure
    class toreDSdef {
	public:
	    string name;	//!< name of the measure, would appear in legends
	    DSkind kind;	//!< nature of the measure
	    toreDSdef (const string & name, DSkind kind) :
		name(name), kind(kind) {}
	    ~toreDSdef () {}
    };


    //! inner definition for a tor bank
    class toreBank {
	public:
	    CollectFreqDuration freq;	//!< update frequency (s)
	    size_t nbmeasures;		//!< number of maesures in one cycle
	    size_t bankpaddedsize;	//!< the padded size of the bank in bytes
	    size_t bankdataoffset;	//!< file offset of the begining of the bank
	    time_t creationdate;	//!< when the bank (and repository) was created
					//    is used as a time-offset for indexes calculations
	    int64_t & lastupdate;	//!< the last update
	    char *map;			//!< where the bank is mapped
	    int64_t cycleduration;	//!< how long is a whole bank cycle (s)
	    int nbMPs;			//!< how many measures in each indexed row

	    toreBank (CollectFreqDuration freq,
			size_t nbmeasures,
			size_t bankpaddedsize,
			size_t bankdataoffset,
			time_t creationdate,
			int64_t *plastupdate,
			int nbMPs
		     );
	    ~toreBank ();
	    int map_it (int fd, bool check=true);
	    int unmap_it (void);

	    int insertvalue (time_t t, list<double> const & lv);
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
	    int nbMPs;		//!< the number of measures in a row - JDJDJDJD not sure MP stands correct here
	time_t creationdate;    //!< creation date of the archive
	int64_t *plastupdate;   //!< pointer to the mapped last update time of the archive

	    //! which measures are in each rows
	    list <toreDSdef> DSdefs;
	    //! current list of banks
	    list<toreBank*> lbanks;

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

	    int specify (             int basetime,
		list<CollectFreqDuration> &lfreq,
			     string const &DSdefinition  );

	    inline time_t lastupdate (void) {
		return (time_t) *plastupdate;
	    }
	    int insertvalue (time_t t, list<double> const & lv);

	private:
	    int readheader (void);
	    int unmapheader (void);
	    int mapallbanks (bool check=true);
	    int unmapallbanks (void);

    };

}  // namespace qiconn


#endif // TORE_H_INCLUDE
