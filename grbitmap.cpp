
#include <iostream>

#include <stdlib.h> // malloc
#include <math.h>   // M_PI cos sin

#include "grbitmap.h"
#include "jeuchar.h"

namespace bulkrays {

    using namespace std;

    GRBitmap::GRBitmap (int w, int h) :
	rawscreen (NULL),
	texturew (w),
	textureh (h),
	drawmode (OPAQUE),
	currentwpixel (0xffffffff),
	currenttransppixel (0xffffffff),
	cur_r (255),
	cur_g (255),
	cur_b (255),
	TranspMask (256),
	ComplTranspMask (0)
    {	rawscreen = (uint8_t *) malloc (w*h*4*sizeof (uint8_t));
	if (rawscreen == NULL) {
	    cerr << "could not allocate " << w << "x" << h << " GRBitmap" << endl;
	    return;
	}
    }

    GRBitmap::~GRBitmap () {
	if (rawscreen != NULL)
	    free (rawscreen);
    }

    void GRBitmap::setdrawmode (DrawMode d) {
	drawmode = d;
    }


    int GRBitmap::dumppng (ostream &out) {
	return pngwrite (texturew, textureh, (char *)rawscreen, out);
    }

    void GRBitmap::computecurrentwpixel () {
	currentwpixel = (cur_r & 0xff) | ((cur_b & 0xff)<<16) | ((cur_g&0xff)<<8) | (0xff<<24);
	currenttransppixel =	  (((cur_r & 0xff) * TranspMask) >> 8)
			      |  ((((cur_b & 0xff) * TranspMask) >> 8) <<16)
			      |  ((((cur_g & 0xff) * TranspMask) >> 8) <<8)
			      |  (0xff                                 <<24);
    }

    void GRBitmap::setcolor (int r, int g, int b) {
	// currentwpixel = ((r & 0xff)<<24) | ((g & 0xff)<<16) | ((b&0xff)<<8) | 0xff;
	cur_r = r;
	cur_g = g;
	cur_b = b;
    
	computecurrentwpixel ();
    }

    void GRBitmap::settransparency (int t) {
	if (t<0) 
	    t = 0;
	else if (t>256)
	    t = 256;

	TranspMask = t;
	ComplTranspMask = 256-t;
	computecurrentwpixel ();
    }

    void GRBitmap::fputpixel (int x, int y, int r, int g, int b) {
	uint8_t *p = rawscreen + ((x+texturew*y) << 2);
	switch (drawmode) {
	    case OPAQUE:
		*p++ = r;
		*p++ = g;
		*p++ = b;
		*p++ = 255;
		break;
	    case TRANSPARENT:
		*p = ((r * TranspMask) >> 8) + ((*p * ComplTranspMask) >> 8); p++;
		*p = ((g * TranspMask) >> 8) + ((*p * ComplTranspMask) >> 8); p++;
		*p = ((b * TranspMask) >> 8) + ((*p * ComplTranspMask) >> 8); p++;
		*p++ = 255;
		break;
	}
	// autoupdatetexture ();
    }

    void GRBitmap::fputpixel (int x, int y) {
	uint32_t *p = (uint32_t *) (rawscreen + ((x+texturew*y) << 2));
	switch (drawmode) {
	    case OPAQUE:
		*p = currentwpixel;
		break;
	    case TRANSPARENT:
		{   uint32_t s = currenttransppixel;
		    uint8_t *pp = (uint8_t*) p;
		    s += ((*pp * ComplTranspMask) >>8)		; pp++;
		    s += ((*pp * ComplTranspMask) >>8)<<8	; pp++;
		    s += ((*pp * ComplTranspMask) >>8)<<16	; pp++;
		    *p = s;
		}
		break;
	}
	// autoupdatetexture ();
    }

    void GRBitmap::putpixel (int x, int y, int r, int g, int b) {
	if ((x<0) || (y<0) || (x>=texturew) || (y>=textureh)) return;
	fputpixel (x,y,r,g,b);
    }

    void GRBitmap::putpixel (int x, int y) {
	if ((x<0) || (y<0) || (x>=texturew) || (y>=textureh)) return;
	fputpixel (x,y);
	// autoupdatetexture ();
    }

    void GRBitmap::clear (void) {
	uint32_t *p = (uint32_t*) rawscreen;
	int nbpix=texturew*textureh;
	for (int i=0 ; i<nbpix ; i++) *p++ = 0;
	// autoupdatetexture ();
    }

//	    void rhline (int x1, int y1, int x2, int y2, uint32_t pixel) {
//		if ((y1<0) || (y1>=textureh)) return;
//		if (x1 > x2) {
//		    rhline (x2, y2, x1, y1, pixel);
//		    return;
//		}
//		if (x2 < 0) return;
//		if (x1 >= texturew) return;
//		if (x1 < 0) x1 = 0;
//		if (x2 >= texturew) x2 = texturew;
//		uint32_t *p = (uint32_t*)(rawscreen + ((x1+texturew*y1) << 2));
//		for (; x1<x2 ; x1++)
//		    *p++ = pixel;
//		autoupdatetexture ();
//	    }
//	
//	    void rvline (int x1, int y1, int x2, int y2, uint32_t pixel) {
//		if ((x1 < 0) || (x1 >= texturew)) return;
//		if (y1 > y2) {
//		    rvline (x2, y2, x1, y1, pixel);
//		    return;
//		}
//		if (y2 < 0) return;
//		if (y1 >= textureh) return;
//		if (y1 < 0) y1 = 0;
//		if (y2 >= textureh) y2 = textureh-1;
//		uint32_t *p = (uint32_t*)(rawscreen + ((x1+texturew*y1) << 2));
//		for (; y1<y2 ; y1++) {
//	//	    if ((y1>=0) && (y1<textureh))
//			*p = pixel;
//		    p += 640;
//		}
//		autoupdatetexture ();
//	    }

    void GRBitmap::rectangle (int x0, int y0, int x1, int y1) {
	if ((x0 < 0) && (x1 < 0)) return;
	if ((y0 < 0) && (y1 < 0)) return;
	if ((x0 >= texturew) && (x1 >= texturew)) return;
	if ((y0 >= textureh) && (y1 >= textureh)) return;

	if (x0>x1) { int t=x0; x0=x1; x1=t; }
	if (y0>y1) { int t=y0; y0=y1; y1=t; }

	if (x0<0) x0=0;
	if (x0>=texturew) return;
	if (x1>=texturew) x1=texturew-1;
	if (y0<0) y0=0;
	if (y0>=textureh) return;
	if (y1>=textureh) y1=textureh-1;

	uint32_t *p;
	switch (drawmode) {
	    case OPAQUE:
		for (int y=y0 ; y<=y1 ; y++) {
//cerr << "y=" << y << endl;
		    p = (uint32_t *) (rawscreen + ((x0+texturew*y) << 2));
		    for (int i=x0 ; i<=x1 ; i++)
			*p++ = currentwpixel;
		}
		break;
	    case TRANSPARENT:
		for (int y=y0 ; y<=y1 ; y++) {
		    p = (uint32_t *) (rawscreen + ((x0+texturew*y) << 2));
		    for (int i=x0 ; i<=x1 ; i++)
		    {   uint32_t s = currenttransppixel;
			uint8_t *pp = (uint8_t*) p;
			s += ((*pp * ComplTranspMask) >>8)		; pp++;
			s += ((*pp * ComplTranspMask) >>8)<<8	; pp++;
			s += ((*pp * ComplTranspMask) >>8)<<16	; pp++;
			*p++ = s;
		    }
		}
		break;
	}
	// autoupdatetexture ();
	
    }

    void GRBitmap::line (int x0, int y0, int x1, int y1) {
	if ((x0 < 0) && (x1 < 0)) return;
	if ((y0 < 0) && (y1 < 0)) return;
	if ((x0 >= texturew) && (x1 >= texturew)) return;
	if ((y0 >= textureh) && (y1 >= textureh)) return;

	int dx = abs(x1-x0);
	int dy = abs(y1-y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2;

	while (true) {
	    if ((x0>=0) && (y0>=0) && (x0<texturew) && (y0<textureh))
		fputpixel (x0,y0);
	    if ((x0 == x1) && (y0 == y1)) break;

	    int e2 = err;
	    if (e2 > -dx) { 
		err -= dy; x0 += sx;
	    }
	    if (e2 < dy) {
		err += dx; y0 += sy;
	    }
	}
	if ((x0>=0) && (y0>=0) && (x0<texturew) && (y0<textureh))
	    fputpixel (x0,y0);
	// autoupdatetexture ();
    }

    void GRBitmap::circle (int xc, int yc, double r) {
	double a;
	int x1 = xc+r, y1=yc;

	// stoprefresh ();
	for (a=0.01 ; a < 2*M_PI ; a+= 0.01) {
	    int x2 = xc + r*cos(a);
	    int y2 = yc + r*sin(a);
	    line (x1,y1, x2,y2);
	    x1=x2, y1=y2;
	}
	line (x1,y1, xc+r,yc);
	// startrefresh ();
    }

    int Random (int max) {
        return (int)(((long int)random () * max)/RAND_MAX);
    }


    void GRBitmap::putstr (int x, int y, uint32_t pixel, string const &texte) {

	int minx, miny, maxx, maxy;

	if ((x>texturew) || (y>textureh))
	    return ;

	// minx = surface->clip_rect.x,
	// miny = surface->clip_rect.y,
	// maxx = minx + surface->clip_rect.w,
	// maxy = miny + surface->clip_rect.h;

	minx = 0;
	miny = 0;
	maxx = texturew-1;
	maxy = textureh-1;

	
	{	int t,g;
	    int l = texte.size();
	    int p = 0;
    //	int ominx = (x<0) ? 0 : x;
    //	int ominy = (y<0) ? 0 : y;
	    int omaxx = x + l * 8;
	    int omaxy = y + 16;

	    if ((omaxx < minx) || (omaxy < miny))
		return ;

	    // if ( SDL_MUSTLOCK(surface) ) {
	    //     if ( SDL_LockSurface(surface) < 0 ) {
	    //         /* JDJDJDJD fprintf(stderr, "Can't lock surface: %s\n", SDL_GetError()); */
	    //         fprintf(stderr, "Can't lock surface: \n");
	    //         return;
	    //     }
	    // }
	    
	    for (p=0 ; p<l ; p++) {
		for (t=0 ; t<16 ; t++)
		    if (((t+y)>=miny) && ((t+y)<maxy)) {
			for (g=0 ; g<8 ; g++)
			    if (((x+g)>=minx) && ((x+g)<maxx))
				if ( HIPjeuchar[ (int)(unsigned char)texte[p] ][t] & (0x80 >> g) )
				    putpixel (x+g,y+t);
		    }
		x += 8;
		if (x > maxx) break;
	    }

    //	SDL_UpdateRect(surface, ominx, ominy, omaxx-ominx, omaxy-ominy);
	    
	    // if ( SDL_MUSTLOCK(surface) ) {
	    //     SDL_UnlockSurface(surface);
	    // }
	}
    }


}

