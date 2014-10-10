
#include <fcntl.h>	    // open
#include <unistd.h>	    // close (!)
#include <errno.h>
#include <string.h>	    // strerror
#include <sys/mman.h>

#include <iostream>
#include <iomanip>

#include "tore.h"

namespace qiconn {


    size_t Tore::pagesize = getpagesize();

    // fills the current file descriptor with "s" zeros, returns the number of byte written or negative
    size_t pad (int fd, size_t s, char padvalue = 0) {
	char buf[1024];
	size_t written = 0;

	if (s == 0)
	    return 0;

	for (int i=0 ; i<1024 ; i++) buf[i] = padvalue;

	while (s > 0) {
	    size_t n = (s > 1024) ? 1024 : s;
	    size_t r = write (fd, buf, n);
	    if (r<0) {
		cerr << "qiconn::pad (tore.cpp) we'll see later what to do here" << endl;
		break;
	    }
	    s -= r;
	    written += r;
	}
	if (written != 0)
	    return written;
	else
	    return -1;
    }

    /*
     *	----------------------------------- DSkind ------------------------------------
     */

    DSkind stoDSkind (const string & s) {
	if ("GAUGE" == s) {
	    return DSK_GAUGE;
	} else if ("COUNTER" == s) {
	    return DSK_COUNTER;
	} else if ("DERIVE" == s) {
	    return DSK_DERIVE;
	} else if ("ABSOLUTE" == s) {
	    return DSK_ABSOLUTE;
	} else {
	    return DSK_UNKNOWN;
	}
    }

    ostream& operator<< (ostream& cout, DSkind const & kind) {
	switch (kind) {
	    case DSK_UNKNOWN:   return cout << "UNKNOWN";
	    case DSK_GAUGE:	return cout << "GAUGE";
	    case DSK_COUNTER:   return cout << "COUNTER";
	    case DSK_DERIVE:    return cout << "DERIVE";
	    case DSK_ABSOLUTE:  return cout << "ABSOLUTE";
	    default:
		cerr << "tore::DSkind warning : encountered some garbaged DSkind at ostream" << endl;
		return cout << "GARBAGED";
	}
    }

    /*
     *	----------------------------------- toreBank ----------------------------------
     */

    toreBank::toreBank (CollectFreqDuration freq,
			long nbmeasures,
			size_t bankpaddedsize,
			size_t bankdataoffset,
			time_t creationdate,
			int64_t *plastupdate
		       ) :
	freq (freq),
	nbmeasures (nbmeasures),
	bankpaddedsize (bankpaddedsize),
	bankdataoffset (bankdataoffset),
	creationdate (creationdate),
	lastupdate (*plastupdate)
    {	
    }

    toreBank::~toreBank () {}

    /*
     *	----------------------------------- Tore --------------------------------------
     */

    Tore::Tore (string filename) : 
	usable (false),
	defined (false),
	fd (-1),
	filename (filename),
	mheader (NULL),
	headersize (0)
    {

	fd = open (filename.c_str(), O_RDWR);

	if (fd<0) {	// either the file doesn't exist or not readable
	    usable = false;
	    return;
	}
	if (readheader () < 0) {
	    cerr << "Tore::" << filename << " : failed at construction" << endl;
	    usable = false;
	    return;
	}
	usable = true;
    }

// file offsets
#define BASETIME_OFF	4	// basetime (s)
#define NBMP_OFF	8	// number of measure-points
#define NBBANK_OFF	12	// number of banks
#define OFFDSDEF0	16	// offset for the first DS definition
#define OFFBANK0	20	// offset for the first bank definition
#define CREATIONDATE	24	// int64 time_t creation of this archive
#define LASTUPDATE  	32	// int64 time_t last update of this archive

// offset valid only at creation time (may change any at time)
#define PROPOFFDSDEF0	64	// offset for the first DS definition
#define DS_DEF_SIZE	64	// total size of a DS definition
#define BK_DEF_SIZE	64	// total size of a bank definition

// offsets inside DS definitions
#define DS_DEFSIZE_OFF	0	// size of DS definitions (incl this size)
#define DS_NAME_OFF	4	// name of the DS
#define DS_NAME_MAXLEN	47	// maximum length for a DSname
#define DS_KIND_OFF	52	// kind of the DS

// offsets inside bank definitions
#define BK_DEFSIZE_OFF	0	// size of bank definitions (incl this size)
#define BK_INTERVAL	4	// update interval for this bank (s)
#define BK_DURATION	8	// duration of on bank revolution
#define BK_NBMEASURES	12	// number of measures
#define BK_SIZE_LL	16	// int64_t padded size of the bank in bytes
#define BK_DATAOFFSET	32	// int64_t offset for datas
#define BK_CREATIONDATE	40	// the 64b timestamp of first data
#define BK_LASTUPDATE  	48	// the 64b timestamp of last update



    int Tore::readheader ()
    {
cerr << "entering readheader" << endl;
	if (pagesize < 4096)
	    headersize = 4096;
	else
	    headersize = pagesize;

	mheader = (char *) mmap (NULL, headersize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
	if ((mheader == MAP_FAILED) || (mheader == NULL)) {
	    int e = errno;
	    cerr << "Tore::" << filename << " Tore::readheader failed could not mmap header: " << strerror (e) << endl;
	    close (fd);
	    return -5;
	}

	// minimal first checks
	// IFF tag
	if ((mheader[0]!='T') || (mheader[1]!='o') || (mheader[2]!='r') || (mheader[3]!='e')) {
	    cerr << "Tore::" << filename << " Tore::readheader failed bad IFF header" << endl;
	    munmap (mheader, headersize); mheader = NULL; close (fd);
	    return -5;
	}
	
	// collection of main properties
	basetime = *(int32_t *)(mheader + BASETIME_OFF);
	nbMPs = *(int32_t *)(mheader + NBMP_OFF);
	nbbanks = *(int32_t *)(mheader + NBBANK_OFF);
cerr << "base pulse = "	<< basetime << "s" << endl
     << nbbanks << " banks of " << nbMPs << " measure-points rows" << endl;
	startDSdef = mheader + *(int32_t *)(mheader + OFFDSDEF0);
	startbankdef = mheader + *(int32_t *)(mheader + OFFBANK0);
	creationdate = (time_t) *(int64_t *)(mheader + CREATIONDATE);
	plastupdate = (int64_t *)(mheader + LASTUPDATE);

	// collection of DS properties
	{   int i;
	    size_t offset = 0;
	    for (i=0 ; i<nbMPs ; i++) {
		string name (startDSdef + offset + DS_NAME_OFF);
		DSkind kind = (DSkind) *(int32_t *)(startDSdef + offset + DS_KIND_OFF);
		DSdefs.push_back (toreDSdef(name, kind));
		offset += *(int32_t *)(startDSdef + offset + DS_DEFSIZE_OFF);
cerr << "   DS:" << name << ":[" << kind << "]" << endl;
	    }
	}

	// collection of bank properties
	{   size_t offset = 0;
	    int i;
	    for (i=0 ; i<nbbanks ; i++) {
		long interval = *(int32_t *)(startbankdef + offset + BK_INTERVAL);
		long duration = *(int32_t *)(startbankdef + offset + BK_DURATION);
		long nbmeasures = *(int32_t *)(startbankdef + offset + BK_NBMEASURES);
		size_t bankpaddedsize = (size_t)(*(int64_t *)(startbankdef + offset + BK_SIZE_LL));
		size_t bankdataoffset = (size_t)(*(int64_t *)(startbankdef + offset + BK_SIZE_LL));
		time_t creationdate = (time_t) (*(int64_t *)(startbankdef + offset + BK_CREATIONDATE));
		int64_t *plastupdate = (int64_t *)(startbankdef + offset + BK_LASTUPDATE);
		offset += *(int32_t *)(startbankdef + offset + BK_DEFSIZE_OFF);

		toreBank *pbank = new toreBank (CollectFreqDuration(interval,duration),
						nbmeasures,
						bankpaddedsize,
						bankdataoffset,
						creationdate,
						plastupdate
						);
		if (pbank == NULL) {
		    cerr << "Tore::" << filename << " Tore::readheader failed. could not allocate toreBank" << endl;
		    return -7;
		}
		lbanks.push_back (pbank);

cerr << "   freq : " << interval << " x " << duration << "  = " << nbmeasures << " measures" << endl
     << "          padded = " << bankpaddedsize << " (" << bankpaddedsize/1024 << "Ko),  off=" << bankdataoffset << endl
     << endl;
	    }
	}

	return 0;
    }

    int Tore::mapall (void) {
	return 0;
    }

    Tore::~Tore () {
// JDJDJDJD MISSING should unmap the mapping !!!!!
	if (fd > 0) close (fd);
    }

    int Tore::specify (int basetime, list<CollectFreqDuration> &lfreq, string const &DSdefinition) {

	Tore::basetime = basetime;

	if (defined) {
	    cerr << "Tore::" << filename << " : attempt to redefine" << endl;
	    return -6;
	}
	
	fd = open (filename.c_str(), O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if (fd<0) {	// either the file doesn't exist or not readable
	    int e = errno;
	    cerr << "Tore::" << filename << " Tore::specify failed : " << strerror (e) << endl;
	    usable = false;
	}

	if (pagesize < 4096)
	    headersize = 4096;
	else
	    headersize = pagesize;

	pad (fd, headersize, 0x7e);

	mheader = (char *) mmap (NULL, headersize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
	if ((mheader == MAP_FAILED) || (mheader == NULL)) {
	    int e = errno;
	    cerr << "Tore::" << filename << " Tore::specify failed could not mmap header: " << strerror (e) << endl;
	    close (fd);
	    return -5;
	}

	// write the IFF tag
	mheader[0]='T'; mheader[1]='o'; mheader[2]='r'; mheader[3]='e';

	*(int32_t *)(mheader + BASETIME_OFF) = basetime;
	

	list <string> DSnames;
	list <string> DSkinds;

	string const &s = DSdefinition;
	size_t p=0, q=0;
	while (p != string::npos) {
	    p = s.find (':', p); if (p == string::npos) break;
	    p++;
	    q = s.find (':', p);
	    if (q == string::npos) {
		DSnames.push_back (s.substr(p));
		break;
	    }
	    DSnames.push_back (s.substr(p, q-p));

	    p = q+1;
	    q = s.find (':', p);
	    if (q == string::npos) {
		DSkinds.push_back (s.substr(p));
		break;
	    }
	    DSkinds.push_back (s.substr(p, q-p));

	    p = q;
	    p = s.find (':', p+1); if (p == string::npos) break;
	    p = s.find (':', p+1); if (p == string::npos) break;
	    p = s.find (':', p+1); if (p == string::npos) break;
	}

	list<string>::iterator li;
	for (li=DSnames.begin() ; li!=DSnames.end() ; li++)
	    cout << "     [" << *li << "]" << endl;
	cout << endl;

	for (li=DSkinds.begin() ; li!=DSkinds.end() ; li++)
	    cout << "     [" << *li << "]" << endl;
	cout << endl;


	if (DSkinds.size() != DSnames.size()) {
	    cerr << "Tore::" << filename << " Tore::specify failed : nb names(" << DSnames.size()
		 << ") and nb kinds(" << DSkinds.size() << ") differs" << endl; 
	    return -2;
	}

	if (DSnames.empty()) {
	    cerr << "Tore::" << filename << " Tore::specify failed : no DS names found" << endl;
	    return -3;
	}

	creationdate = time(NULL);

	// writing the number of measure-points
	*(int32_t *)(mheader + NBMP_OFF) = DSnames.size();

	// writing the number of banks
	// JDJDJDJD maybe it should be better to validate them before ?
	*(int32_t *)(mheader + NBBANK_OFF) = lfreq.size();

	// writing first DS definition offset position
	*(int32_t *)(mheader + OFFDSDEF0) = PROPOFFDSDEF0;

	// writing first DS definition offset position
	*(int32_t *)(mheader + OFFDSDEF0) = PROPOFFDSDEF0;

	size_t offset_defbank0 = PROPOFFDSDEF0 + DSnames.size() * DS_DEF_SIZE;
	// writing first bank definition offset position
	*(int32_t *)(mheader + OFFBANK0) = offset_defbank0;
	// writing creation date
	*(int64_t *)(mheader + CREATIONDATE) = creationdate;
	// writing last update as unkonwn
	*(int64_t *)(mheader + LASTUPDATE) = TORE_TIME_UNKNOWN;
	

	// writing DS names and def sizes
	{   int dsnum;
	    list<string>::iterator li;
	    for (dsnum=0,li=DSnames.begin() ; li!=DSnames.end() ; li++,dsnum++) {
		string &name = *li;
		size_t i;
		for (i=0 ; (i<DS_NAME_MAXLEN) && (i<name.size()) ; i++) {
		    *(mheader + PROPOFFDSDEF0 + dsnum*DS_DEF_SIZE + DS_NAME_OFF + i) = name[i];
		}
		    *(mheader + PROPOFFDSDEF0 + dsnum*DS_DEF_SIZE + DS_NAME_OFF + i) = 0;

		*(int32_t *)(mheader + PROPOFFDSDEF0 + dsnum*DS_DEF_SIZE + DS_DEFSIZE_OFF) = DS_DEF_SIZE;

		if (i >= DS_NAME_MAXLEN) {	// JDJDJDJD a previous check should prevent this
		    cerr << "Tore::" << filename << " Tore::specify warning : DS names \"" << name << "\" is too long and was trunked" << endl;
		}
	    }
	}



	// writing DS kinds
	{   int dsnum;
	    list<string>::iterator li;
	    for (dsnum=0,li=DSkinds.begin() ; li!=DSkinds.end() ; li++,dsnum++) {
		DSkind kind = stoDSkind (*li);

		*(int32_t *)(mheader + PROPOFFDSDEF0 + dsnum*DS_DEF_SIZE + DS_KIND_OFF) = (int32_t)kind;

		if (kind == DSK_UNKNOWN) {	// JDJDJDJD a previous check should prevent this
		    cerr << "Tore::" << filename << " Tore::specify warning : unkown kind \"" << *li << "\"" << endl;
		}
	    }
	}

	// writing Bank definitions and offsets
	size_t bigdataoffset = headersize;
	{
cerr << "bigdataoffset = " << bigdataoffset << endl;

	    list<CollectFreqDuration>::iterator lj;
	    int banknum = 0;
	    for (lj=lfreq.begin(),banknum=0 ; lj!=lfreq.end() ; lj++, banknum++) {
		size_t bankoffset = offset_defbank0 + banknum * BK_DEF_SIZE;
		// write defsize, interval, duration
		*(int32_t *)(mheader + bankoffset + BK_DEFSIZE_OFF) = BK_DEF_SIZE;
		*(int32_t *)(mheader + bankoffset + BK_INTERVAL) = lj->interval;
		*(int32_t *)(mheader + bankoffset + BK_DURATION) = lj->duration;
		*(int32_t *)(mheader + bankoffset + BK_NBMEASURES) = lj->duration / lj->interval;
		size_t datasize = (lj->duration / lj->interval) * (8 * DSnames.size());
		// check if it's a multiple of pagesize or pad
		size_t padsize;
		if (datasize % pagesize == 0)
		    padsize = datasize;
		else
		    padsize = pagesize * (1 + datasize/pagesize);
		*(int64_t *)(mheader + bankoffset + BK_SIZE_LL) = padsize;
		*(int64_t *)(mheader + bankoffset + BK_DATAOFFSET) = bigdataoffset;
		*(int64_t *)(mheader + bankoffset + BK_CREATIONDATE) = creationdate;
		*(int64_t *)(mheader + bankoffset + BK_LASTUPDATE) = TORE_TIME_UNKNOWN;
		bigdataoffset += padsize;
cerr << "bigdataoffset = " << bigdataoffset << endl;
	    }
	}

	cout << "sizes to allocate : " << endl;
	list<CollectFreqDuration>::iterator lj;
	int n;
	for (lj=lfreq.begin(),n=0 ; lj!=lfreq.end() ; lj++, n++) {
	    cout << "   bank " << n << " (" << lj->interval << "x" << lj->duration << "): " << 8*DSnames.size() * (lj->duration/lj->interval) << endl;

	    // write name in header
	    
	}

	{   cout << "padding the repository :" << endl;
	    size_t towrite = bigdataoffset - headersize;
	    while (towrite > 0) {
		size_t size = (towrite > 128*1024) ? 128*1024 : towrite;
		size_t w = pad (fd, size);
		if (w > 0)
		    towrite -= w;
		else {
		    int e = errno;
		    cerr <<  "Tore::" << filename << " Tore::specify error in padding : " << strerror (e) << endl;
		    return -4;
		}
		cout << "  " << 100 - ((100*towrite) / (bigdataoffset-headersize)) << "%\r" << flush;
	    }
	    cout << "  100%" << endl;
	}

	return 0;
    }


} // namespace qiconn
