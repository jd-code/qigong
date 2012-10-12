#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>


#include <string>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

#define QICONN_H_GLOBINST
#include <qiconn/qiconn.h>

using namespace std;
using namespace qiconn;

namespace std {

    int isfullnum (const char * name, int n) {
	if ((name == NULL) || (n==0))
	    return 0;

	while ( (*name != 0) && (n>0)) {
	    if (isdigit (*name))
		name++, n--;
	    else
		return 0;
	}
	return 1;
    }

    int killall (char ** name) {
	int n = 0;
	DIR * sproc = opendir ("/proc");
	struct dirent *de;

	if (sproc == NULL) {
	    int e = errno;
	    syslog (LOG_ERR, "could not open /proc for killing : %s", strerror (e));
	    return 0;
	}

	while ((de = readdir (sproc)) != NULL) {
	    char dirname[NAME_MAX*2+1];
	    FILE *f;
	    char buf [1024];
	    size_t s;
	    char *p, *q;

	    if (!isfullnum (de->d_name, NAME_MAX))
		continue;

	    strncpy (dirname, "/proc/", 7);
	    strncat (dirname, de->d_name, NAME_MAX);
	    strncat (dirname, "/stat", 6);
		
	    if ((f = fopen (dirname, "rt")) == NULL) 
		continue;
	    
	    s = fread (buf, 1, 1023, f);
	    fclose (f);

	    if (s<0)
		continue;

	    buf [s] = 0;
	    if ((p = strchr (buf, '(')) == NULL)
		continue;

	    if ((q = strchr (p, ')')) == NULL)
		continue;

	    {   int i;
		for (i=0 ; name[i] != NULL ; i++) {
		    pid_t pid;
		    if (strncmp (p+1, name[i], q-(p+1)) != 0)
			continue;

		    pid = atol (de->d_name);
		    kill  (pid, SIGKILL);
		    n++;
		}
	    }
	}

	closedir (sproc);
	return n;
    }



    bool debug = false;
    inline char eos(void) { return '\0'; }

    string decport2hexport (const string &port) {
	stringstream b;
	b.setf (ios::uppercase);
	b << setbase(16)
	  << setw(4)
	  << setfill('0')
	  << atol (port.c_str());  // << eos();
	return b.str();
    }

    string ipv4_to_encsv4 (const string &ip) {
	struct in_addr a;
	if (inet_pton (AF_INET, ip.c_str(), &a) < 0) {
	    int e = errno;
	    if (debug) cerr << "could not translate \"" << ip << "\" to an ipv4 address : " << strerror (e) << endl;
	    string s;
	    return s;
	}
	stringstream b;
	b.setf (ios::uppercase);
	// b << "0000000000000000FFFF0000"
	b << setbase(16)
	  << setw(8)
	  << setfill('0')
	  << (long)(a.s_addr); //  << eos();
	return b.str();
    }

    string ipv4_to_encsv6 (const string &ip) {
	struct in_addr a;
	if (inet_pton (AF_INET, ip.c_str(), &a) < 0) {
	    int e = errno;
	    if (debug) cerr << "could not translate \"" << ip << "\" to an ipv4 address : " << strerror (e) << endl;
	    string s;
	    return s;
	}
	stringstream b;
	b.setf (ios::uppercase);
	b << "0000000000000000FFFF0000"
	  << setbase(16)
	  << setw(8)
	  << setfill('0')
	  << (long)(a.s_addr); //  << eos();
	return b.str();
    }

    string ipv6_to_encsv6 (const string &ip) {
	struct in6_addr a;
	if (inet_pton (AF_INET6, ip.c_str(), &a) < 0) {
	    int e = errno;
	    if (debug) cerr << "could not translate \"" << ip << "\" to an ipv6 address : " << strerror (e) << endl;
	    string s;
	    return s;
	}
	int i;
	stringstream b;
	for (i=0 ; i<4 ; i++) {
	    b.setf (ios::uppercase);
	    b << setbase(16)
	      << setw(8)
	      << setfill('0')
	      // JDJDJDJD j'ai converti ca a toute vitesse sans verifier
	      // on avait ca avant :
	      // << ((long)(a.in6_u.u6_addr32[i]));
	      << ((long)(a.__in6_u.__u6_addr32[i]));
	}
	return b.str();
    }


    // string=(addr:port) / inode
    typedef map <string, long> TMAdrPortIno;

    void dump_TMAdrPortIno (TMAdrPortIno & m, ostream &cout) {
	TMAdrPortIno::iterator mi;
	for (mi=m.begin() ; mi!=m.end() ; mi++)
	    cout << mi->first << " " << mi->second << endl ;
    }

    class sockinfo {
	public:
	    char srcadrport[38];
	    char dstadrport[38];
	    sockinfo () {
		srcadrport[0] = 0, dstadrport[0] = 0;
	    }
    };
    // inode // sockinfo
    typedef map <ino_t, sockinfo> TMInoInt;

    void dump_TMInoInt (TMInoInt & m, ostream &cout) {
	TMInoInt::iterator mi;
	for (mi=m.begin() ; mi!=m.end() ; mi++)
	    cout << mi->first << " " << mi->second.srcadrport << "-" << mi->second.dstadrport << endl ;
    }

    TMAdrPortIno madrportino;
    TMInoInt minoint;

    void hash_procnettcp ()
    {
	minoint.erase(minoint.begin(),minoint.end());
	{   ifstream nettcp ("/proc/net/tcp");
	    if (!nettcp) {
		int e = errno;
		cerr << "could not open /proc/net/tcp : " << strerror(e) << endl;
		return;
	    }
	    long n = 0;
	    while (nettcp) {
		string s;
		if (!getstring (nettcp, s))
		    continue;
		n++;
		if (n == 1)
		    continue;

		if (s.substr(20,13) == "00000000:0000") {
		    // listening sockets
		    long inode = atol (s.substr(91, s.find(' ',91) - 91).c_str());
		    if (inode != 0)
			madrportino[s.substr(6,13)] = inode;
		} else {
		    // regular (point to point ?) sockets
		    long inode = atol (s.substr(91, s.find(' ',91) - 91).c_str());
		    sockinfo so;
		    // so.srcadrport = strtol (s.substr(15,4).c_str(), NULL, 16);
		    // so.dstadrport = strtol (s.substr(29,4).c_str(), NULL, 16);
		    strncpy (so.srcadrport, s.c_str()+6, 13); so.srcadrport[13] = 0;
		    strncpy (so.dstadrport, s.c_str()+20, 13); so.dstadrport[13] = 0;
		    if (inode != 0)
			minoint[inode] = so;
		}
	    }
	}
	{   ifstream nettcp ("/proc/net/tcp6");
	    if (!nettcp) {
		int e = errno;
		cerr << "could not open /proc/net/tcp6 : " << strerror(e) << endl;
		return;
	    }
	    long n = 0;
	    while (nettcp) {
		string s;
		if (!getstring (nettcp, s))
		    continue;
		n++;
		if (n == 1)
		    continue;

		if (s.substr(44,37) == "00000000000000000000000000000000:0000") {
		    // listening sockets
		    long inode = atol (s.substr(139, s.find(' ',139) - 139).c_str());
		    if (inode != 0)
			madrportino[s.substr(6,37)] = inode;
		} else {
		    // regular (point to point ?) sockets
		    long inode = atol (s.substr(139, s.find(' ',139) - 139).c_str());
		    sockinfo so;
		    // so.srcport = strtol (s.substr(39,4).c_str(), NULL, 16);
		    // so.dstport = strtol (s.substr(77,4).c_str(), NULL, 16);
		    strncpy (so.srcadrport, s.c_str()+6, 37); so.srcadrport[37] = 0;
		    strncpy (so.dstadrport, s.c_str()+44, 37); so.dstadrport[37] = 0;
		    if (inode != 0)
			minoint[inode] = so;
		}
	    }
	}
    }

    void repartportpid (pid_t pid, int mini=0) {
	    stringstream dirname ;
	    dirname << "/proc/" << (long)(pid) << "/fd";
	    DIR * dir = opendir (dirname.str().c_str());

	    if (dir == NULL) {
		int e = errno;
		cerr << "could not opendir (\"" << dirname << "\") : " << strerror (e) << endl;
		return;
	    }
	    map <string, long> mportuse;
	    struct dirent * pde;
	    while ((pde = readdir (dir)) != NULL) {
		string fname(dirname.str());
		fname += "/";
		fname += pde->d_name;
		struct stat fst;
		if (lstat (fname.c_str(), &fst) != 0) {
		    int e = errno;
		    cerr << "could not stat (\"" << fname << "\") : " << strerror (e) << endl;
		    continue;
		}
		if (S_ISLNK (fst.st_mode)) {
		    
		    char buf [1024];
		    ssize_t len = readlink (fname.c_str(), buf, 1023);
		    if (len != -1) {
		        buf[len] = '\0';
			if (strncmp (buf, "socket:[", 8) == 0) {
			    int socket = atol (buf+8);
			    if (socket == 0)
				continue;
			    TMInoInt::iterator mi = minoint.find(socket);
			    if (mi != minoint.end()) {
				map <string, long>::iterator mj;

				mj = mportuse.find(mi->second.srcadrport);
				if (mj == mportuse.end())
				    mportuse[mi->second.srcadrport] = 1;
				else
				    mj->second ++;

				mj = mportuse.find(mi->second.dstadrport);
				if (mj == mportuse.end())
				    mportuse[mi->second.dstadrport] = 1;
				else
				    mj->second ++;
			    }
// 			    else {
// 				cerr << "unknown socket " << socket << endl;
// 			    }
			}
		    }
		}
	    }
	    closedir (dir);
	    {	map <string, long>::iterator mi;
		for (mi=mportuse.begin() ; mi!=mportuse.end() ; mi++) {
		    if (mi->second > mini)
			cout << setw(10) << mi->first << " " << mi->second << endl;
		}
	    }
    }


    class MatchListenAdress
    {
	public:
	    string addrport1;
	    string addrport2;
	    bool maymatch2;

	    string match_inode;	// the string to match against links in /proc/[pid]/fd/*
				// "socket:[......]"

	    pid_t cur_pid;
	    long cur_inode;
	    string cur_socketlinkname;
	    string cur_procfdlink;


	MatchListenAdress (string addrport) {
	    cur_pid = 0;
	    cur_inode = 0;

	    size_t pos = addrport.rfind(':');
	    if (addrport.find('.') != string::npos) {	// we have an IPv4 addr
		addrport1 = ipv4_to_encsv4 (addrport.substr(0,pos)) + ":" + decport2hexport(addrport.substr(pos+1));
		addrport2 = ipv4_to_encsv6 (addrport.substr(0,pos)) + ":" + decport2hexport(addrport.substr(pos+1));
		maymatch2 = true;
	    } else {
		addrport1 = ipv6_to_encsv6 (addrport.substr(0,pos)) + ":" + decport2hexport(addrport.substr(pos+1));
		maymatch2 = false;
	    }
	}

	TMAdrPortIno::iterator findinode () {
	    TMAdrPortIno::iterator mi = madrportino.find (addrport1);
	    if (mi == madrportino.end () && maymatch2)
		mi = madrportino.find (addrport2);
	    return mi;
	}

	void set_match_inode (long inode) {
	    cur_inode = inode;
	    stringstream tmpss;
	    tmpss << "socket:[" << cur_inode << "]" << eos() ;
	    match_inode = tmpss.str();
	}

	bool findmatchsock () {
	    TMAdrPortIno::iterator mi = findinode ();
	    if (mi == madrportino.end ()) {
		match_inode.erase();
		return false;
	    }
	    set_match_inode ((long)(mi->second));
	    return true;
	}

	bool does_procfdlink_match (string fname) {
	    struct stat fst;
	    if (lstat (fname.c_str(), &fst) != 0) {
		// int e = errno;
		// cerr << "could not stat (\"" << fname << "\") : " << strerror (e) << endl;
		return false;
	    }
	    if (S_ISLNK (fst.st_mode)) {
		char buf [1024];
		ssize_t len = readlink (fname.c_str(), buf, 1023);
		if (len != -1) {
		    buf[len] = '\0';
		    if (strncmp (buf, match_inode.c_str(), len) == 0)
			return true;
		}
	    }
	    return false;
	}

	void setnomatch () {
	    cur_pid = 0;
	    cur_inode = 0;
	    match_inode.erase();
	}

	pid_t get_pidmatch () {
	    TMAdrPortIno::iterator mi = findinode ();
	    if (mi == madrportino.end ()) {	// is there still a socket matching the listen address ?
		setnomatch ();
		return 0;
	    }
	    if (mi->second == cur_inode) {	// did the inode changed ?
		if (does_procfdlink_match (cur_procfdlink))
		    return (cur_pid);
	    }
cout << "  we're rehashing everything here" << endl;
	    set_match_inode ((long)(mi->second));

	    DIR * procdir;
	    procdir = opendir ("/proc");

	    if (procdir == NULL) {
		int e = errno;
		cerr << "could not opendir /proc : " << strerror (e) << endl;
		setnomatch ();
		return 0;
	    }

	    struct dirent * procentry;
	    while ((procentry = readdir (procdir)) != NULL) {
		if (!isdigit (procentry->d_name[0]))
		    continue;
		
		string dirname ("/proc/");
		dirname += procentry->d_name;
		dirname += "/fd";
		DIR * dir = opendir (dirname.c_str());

		if (dir == NULL) {
    ////////		int e = errno;
    ////////		cerr << "could not opendir (\"" << dirname << "\") : " << strerror (e) << endl;
		    continue;
		}

		struct dirent * pde;
		while ((pde = readdir (dir)) != NULL) {
		    string fname(dirname);
		    fname += "/";
		    fname += pde->d_name;

		    if (does_procfdlink_match (fname)) {
			cur_procfdlink = fname;
			cur_pid = atol (procentry->d_name);
			closedir (dir);
			closedir (procdir);
			return cur_pid;
		    }
		}
		closedir (dir);
	    }
	    closedir (procdir);
	    setnomatch ();
	    return 0;
	}
    };
}

int main (int nb, char ** cmde) {

    int delay = 10;
    int restartdelay = 10;
    char ** parg;
    char ** killprocess = NULL;
    int maxconn = 50;

    parg = (char**) malloc ((nb+1) * sizeof (char *));

    if (parg == NULL) {
	cerr <<  "could not allocate buffer for execvp ! - exitingC" << endl ;
	exit (1);
    }

    MatchListenAdress *mla = NULL;

    openlog (cmde[0], LOG_NOWAIT | LOG_PID, LOG_DAEMON);
    
    {	int i;
	bool hasaddressport = false;
	for (i=1 ; i<nb ; i++) {
	    if ((strncmp (cmde[i], "-help", 5) == 0) || (strncmp (cmde[i], "--help", 6) == 0)) {
		cout << cmde[0] << " : follow the process which listens to some adr:port" << endl
		     << "and triggers a valve command when some connection are too numerous" << endl
		     << "usage : " << cmde[0] << " adr:port [-maxconn=m] [-delay=d] [-restartdelay=rd] --killall process names names -- command and params ..." << endl
		     << "        delay : integer, seconds, minimum 1" << endl
		     << "        restartdelay : integer, seconds" << endl;
		return 0;
	    }
	    if ((!hasaddressport) && (cmde[i][0]!='-')) {
		mla = new MatchListenAdress (cmde[i]);
		if (mla == NULL) {
		    cerr << "could not allocate for match listen address \""
			 << cmde[i] << "\" : bailing out" << endl ;
		    return 1;
		}
	    }

	    if (strncmp (cmde[i], "-delay=", 7) == 0) {
		delay = atoi (cmde[i] + 7);
		if (delay < 1) {
		    cerr << "delay setting too low !!" << endl ;
		    delay = 1;
		}
		continue;
	    }
	    if (strncmp (cmde[i], "-maxconn=", 9) == 0) {
		maxconn = atoi (cmde[i] + 9);
		if (maxconn < 1) {
		    cerr << "strange value for maxconn, tuned from " << maxconn << " to 50" << endl ;
		    maxconn = 50;
		}
		continue;
	    }
	    if (strncmp (cmde[i], "-restartdelay=", 14) == 0) {
		restartdelay = atoi (cmde[i] + 14);
		continue;
	    }

	    if (strncmp (cmde[i], "--killall", 10)) {
		killprocess = &cmde[i];
		i++;
		while ((i<nb) && strcmp ("--", cmde[i]) != 0) i++;
		if (i<nb);
		    cmde[i] = NULL;
		i++;
		break;
	    }

	    if (strcmp ("--", cmde[i]) == 0) {
		i++;
		break;
	    }
	}

	if (i >= nb) {
	    cerr << "no command given ?" << endl;
	    exit (1);
	}

	{	int j;
	    for (j=0 ; i<nb ; i++, j++) {
		parg[j] = cmde[i];
	    }
	    parg[j] = NULL;
	}
    }

    {   pid_t child = fork();
        int e = errno;
        switch (child) {
            case -1:
		cerr << "could not fork to background : " << strerror (e) << " - exiting" << endl;
                exit (1);
                break;
            case 0:
		syslog (LOG_INFO, "started : maxconn=%d", maxconn);
                break;
            default:
		cerr << cmde[0] << " started ... pid = " << (long)(child) << endl;
                return 0;
                break;
        }
	if (setsid () == -1) {
	    int e = errno;
	    cerr << "setsid faild, ignored : " <<  strerror (e) << endl;
	}
    }

    if (close (0) != 0) {
	int e = errno;
        cerr << "could not close stdin " << strerror (e) << endl;
        return -1;
    }
    if (freopen ("/dev/null", "a", stdout) == NULL) {
	int e = errno;
	cerr << "could not open /dev/null : " << strerror (e) << endl;
        return -1;
    }
    if (freopen ("/dev/null", "a", stderr) == NULL) {
	int e = errno;
	cerr << "could not open /dev/null : " << strerror (e) << endl;
        return -1;
    }

    while (1) {
	hash_procnettcp ();
	mla->get_pidmatch ();
	int conn = 0;

	if (conn >= maxconn) {

	    if (killprocess != NULL) {
		int i = 0,
		    n;
		
		while (i<10) {
		    n = killall (killprocess);
		    if ((n != 0) || (i == 0)) syslog (LOG_ERR, "maxconn %d >= %d reached, killed %d processes at round %d", conn, maxconn, n, i);
		    if (n == 0)
			break;
		    i++;
		    usleep (250);
		}
	    }

	    pid_t child = fork();
	    switch (child) {
		case -1:
		    syslog (LOG_ERR, "could not fork at crisis !!");
		    break;
		case 0:
		    execvp (parg[0], parg);
		    break;
		default:
		    syslog (LOG_ERR, "maxconn %d >= %d reached, forked for valve command, sleeping %d before watching again", conn, maxconn, restartdelay);
		    sleep (restartdelay);
// JDJDJDJD
//		    syslog (LOG_ERR, "after sleeping %d, load is %d", restartdelay, getload());
		    break;
	    }
	}
	sleep (delay);
    }

    closelog();
    return 0;
}
