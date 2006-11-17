#include <fstream>
#include <iomanip>

#define QICOLLECT_H_GLOBINST
#define QICONN_H_GLOBINST
#include "qicollect.h"

namespace qiconn {
    using namespace std;


    
} // namespace qiconn

using namespace std;
using namespace qiconn;

int main (int nb, char ** cmde) {

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

