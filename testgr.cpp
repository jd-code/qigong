
#include <iostream>
#include <iomanip>
#include <sstream>


#include <list>

#include <math.h>



#include "grbitmap.h"

using namespace std;
using namespace bulkrays;

#define GRMARGIN 4

int sg (GRBitmap &g, list<double> const&l) {
    if (l.empty()) return 0;
    double min=*(l.begin()), max=min;
    list<double>::const_iterator li;

    for (li=l.begin() ; li!=l.end() ; li++) {
	if (*li > max) max = *li;
	if (*li < min) min = *li;
    }

    stringstream l1, l2;
    l1 << min;
    l2 << max; 

    g.clear();
    g.setcolor (255,255,255);
    g.rectangle (0,0, g.texturew, g.textureh);
    g.setcolor (200,0,0);

    g.putstr (GRMARGIN, GRMARGIN, l2.str());
    g.putstr (GRMARGIN, g.textureh-16-GRMARGIN, l1.str());

    size_t len = l1.str().length();
    if (l2.str().length() > len) len = l2.str().length();

    int x1 = GRMARGIN + len * 8 + 8;
    int x2 = g.texturew - GRMARGIN;
    int y1 = g.textureh-8-GRMARGIN;
    int y2 = 8+GRMARGIN;



    if (true)
    {	// spread points
	size_t nbp = l.size();
	int i = 0;
	int lastx = -1;
	int px = -1, py = -1, nx, ny;
	list<double> b;
	for (li=l.begin() ; li!=l.end() ; li++, i++) {
	    int x = x1 + (i*(x2-x1))/nbp;

	    if (x != lastx) {
		if (!b.empty()) {
		    list<double>::iterator lj;
		    double lmin = *(b.begin()), lmax = lmin;
		    double moy =0.0;
		    for (lj=b.begin() ; lj!=b.end() ; lj++) {
			if (*lj > lmax) lmax = *lj;
			if (*lj < lmin) lmin = *lj;
			moy += *lj;
		    }
		    g.setcolor (200,200,200);
		    g.line (	lastx,  y1 + (y2-y1)*(lmin-min)/(max-min),
				lastx,  y1 + (y2-y1)*(lmax-min)/(max-min)
			    );
		    g.setcolor (0,0,0);

		    nx = lastx;
		    ny = y1 + (y2-y1)*((moy/b.size())-min)/(max-min);
		    if (px != -1) {
			g.line(px, py, nx, ny);
		    }
		    px = nx, py = ny;
		    lastx = x;
		}
		b.clear();
		b.push_back (*li);
	    } else {
		b.push_back (*li);
	    }
	}
    }
    if (false)
    {	// spread points
	size_t nbp = l.size();
	int i = 0;
	for (li=l.begin() ; li!=l.end() ; li++, i++) {
	    g.putpixel (	x1 + (i*(x2-x1))/nbp,
			    y1 + (y2-y1)*(*li-min)/(max-min)
			);
	}
    }
    if (false)
    {	// spread points
	size_t nbp = l.size();
	int i = 0;
	int lastx = -1;
	int px = -1, py, nx, ny;
	list<double> b;
	for (li=l.begin() ; li!=l.end() ; li++, i++) {
	    int x = x1 + (i*(x2-x1))/nbp;

	    if (x != lastx) {
		if (!b.empty()) {
		    list<double>::iterator lj;
		    double lmin = *(b.begin()), lmax = lmin;
		    double moy =0.0;
		    for (lj=b.begin() ; lj!=b.end() ; lj++) {
			if (*lj > lmax) lmax = *lj;
			if (*lj < lmin) lmin = *lj;
			moy += *lj;
		    }
//		    g.setcolor (200,200,200);
//		    g.line (	lastx,  y1 + (y2-y1)*(lmin-min)/(max-min),
//				lastx,  y1 + (y2-y1)*(lmax-min)/(max-min)
//			    );

		    nx = lastx;
		    ny = y1 + (y2-y1)*((moy/b.size())-min)/(max-min);
		    if (px != -1) {
			g.setcolor (200,0,0);
			g.line(px, py, nx, ny);
			g.setcolor (0,0,0);
		    }
		    px = nx, py = ny;
		    lastx = x;
		}
		b.clear();
		b.push_back (*li);
	    } else {
		b.push_back (*li);
	    }
	}
    }

    {	double arrmean = pow(10, floor( log(max-min)/log(10) ));
cerr << "range = [" << min << " , " << max << "]" << endl;
cerr << "width = " << max-min << endl;
	double maxarr = arrmean * floor(max / arrmean);
	double minarr = arrmean * ceil(min / arrmean);

	int nbgrad = 1 + (maxarr-minarr)/arrmean;
cerr << "arrd  = [" << minarr << " , " << maxarr << "]" << endl;
cerr << "arrmean = " << arrmean << endl;
cerr << "nbgrad = " << nbgrad << endl;
if (nbgrad <= 2)
	    arrmean /= 5;
	else if (nbgrad <= 4)
	    arrmean /= 2;

	maxarr = arrmean * floor(max / arrmean);
	minarr = arrmean * ceil(min / arrmean);
	nbgrad = 1 + (maxarr-minarr)/arrmean;

cerr << "arrd  = [" << minarr << " , " << maxarr << "]" << endl;
cerr << "arrmean = " << arrmean << endl;
cerr << "nbgrad = " << nbgrad << endl;
	{   int i=0;
	    g.setcolor (0,0,0);
	    double grad = minarr + i * arrmean;
	    //for (i=0 ; i<nbgrad ; i++)
	    while (grad < max) {
cerr << "     " << setw(5) << grad << " - " << endl;
		stringstream label;
		label << setw(5) << grad;
		int y = y1 + (y2-y1)*(grad-min)/(max-min);
		g.putstr (x1-5*8-8, y-8, label.str());
		g.line   (x1-5, y, x1-1, y);
		i++;
		grad = minarr + i * arrmean;
	    }
	}
	g.line (x1-1,y1, x1-1, y2);
	{   int y = y1 + (y2-y1)*(-min)/(max-min);
	    if ((y<=y1) && (y>=y2)) {
		g.line (x1,y,x2,y);	// zero axis
	    }
	}
    }
    


    return 0;
}

int main (void) {
    // GRBitmap g(640,200);
    GRBitmap g(1024,400);

    if (!g) {
	cerr << "the bitmap is unusable !" << endl;
	return -1;
    }


    list<double> l;

    {	int i;
#define NBSPOTS 10000.0
	for (i=0 ; i<NBSPOTS ; i++) {
	    // l.push_back (521 * cos(3.0*  14.3*i/NBSPOTS) + sin (3.0*  (0.34+23.2*i/NBSPOTS)) + ((Random(1000)-500)/500.0)*((Random(1000)-500)/500.0) );
	    l.push_back ( 1+ 0.002 * (cos(3.0*  14.3*i/NBSPOTS) + sin (3.0*  (0.34+23.2*i/NBSPOTS)) + ((Random(1000)-500)/500.0)*((Random(1000)-500)/500.0)) );
	    //l.push_back (4 + 2 * cos(3.0*  14.3*i/NBSPOTS) + sin (3.0*  (0.34+23.2*i/NBSPOTS)) + ((Random(1000)-500)/500.0)*((Random(1000)-500)/500.0) );
	}
    }
    sg (g, l);

    g.dumppng (cout);
    return 0;


    string s("bala bala bala !");

    g.clear();
    g.setcolor (255,255,255);
    g.rectangle (0,0, 640,480);
    g.setcolor (0,0,0);

    int i,j;
    for (i=0 ; i<256 ; i+= 32) {
	string s;
	for (j=0 ; j<32 ; j++) s += (char)(i+j);

	g.putstr (20,20+i/2, s);
    }

    g.dumppng (cout);


    return 0;
}
