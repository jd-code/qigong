
#include <iostream>
#include <iomanip>

#include "tore.h"

using namespace std;
using namespace qiconn;

int main (void) {

    Tore tore("bigbagboug.tore");
    tore.mapall (true);

    if (tore)
	cout << "tore reports ok" << endl;
    else
	cout << "tore reports not ok" << endl;

    if (!tore)
	cout << "tore reports not ok (via not)" << endl;
    else
	cout << "tore reports ok (via not)" << endl;

    {
	list<CollectFreqDuration> lfreq;
	lfreq.push_back (CollectFreqDuration(10, 60480));    // 10s x 7j
	lfreq.push_back (CollectFreqDuration(60, 604800));
	lfreq.push_back (CollectFreqDuration(300, 5529600));
	lfreq.push_back (CollectFreqDuration(3600, 63244800));



	string DSdefinition;

	DSdefinition += "DS:mem_free:GAUGE:20:0:U";    DSdefinition += '\0';
	DSdefinition += "DS:mem_buffers:GAUGE:30:0:U"; DSdefinition += '\0';
	DSdefinition += "DS:mem_cached:GAUGE:40:0:U";  DSdefinition += '\0';
	DSdefinition += "DS:mem_used:GAUGE:50:0:U";    DSdefinition += '\0';
	DSdefinition += "DS:mem_sw_used:GAUGE:60:0:U"; DSdefinition += '\0';
	DSdefinition += "DS:mem_sw_free:GAUGE:70:0:U"; DSdefinition += '\0';


	tore.specify (10, lfreq, DSdefinition);
    }

    return 0;
}
