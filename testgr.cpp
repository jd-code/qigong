
#include <iostream>
#include <sstream>

#include "grbitmap.h"

using namespace std;
using namespace bulkrays;

int main (void) {
    GRBitmap g(640,200);

    if (!g) {
	cerr << "the bitmap is unusable !" << endl;
	return -1;
    }

    string s("bala bala bala !");

    g.clear();
    g.setcolor (255,255,255);
    g.rectangle (0,0, 640,480);
    g.setcolor (0,0,0);

    int i,j;
    for (i=0 ; i<256 ; i+= 32) {
	string s;
	for (j=0 ; j<32 ; j++) s += (char)(i+j);

	g.putstr (20,20+i/2, 10, s);
    }

    g.dumppng (cout);


    return 0;
}
