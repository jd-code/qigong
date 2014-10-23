#ifndef GRBITMAP_H_INCLUDE
#define GRBITMAP_H_INCLUDE

#include <string>

#include <stdint.h>

#include "pngmembuf.h"

namespace bulkrays {

    typedef enum {
        OPAQUE,
        TRANSPARENT
    } DrawMode;

    class GRBitmap {

	public:

        uint8_t	*rawscreen;

	    int	texturew,
		textureh;

       DrawMode	drawmode;
	 uint32_t	currentwpixel;
	 uint32_t	currenttransppixel;
	 uint16_t	cur_r;
	 uint16_t	cur_g;
	 uint16_t	cur_b;
	 uint16_t	TranspMask;
	 uint16_t	ComplTranspMask;

	GRBitmap (int w, int h);
	~GRBitmap ();
	int dumppng (std::ostream &out);
	// are we usable ?
	inline operator const void * () const {
	    return (rawscreen != NULL) ? this : NULL;
	}

	// not(are we usable ?)
	inline bool operator ! () const {
	    return (rawscreen != NULL) ? false : true;
	}

	void setdrawmode (DrawMode d);
	void computecurrentwpixel ();
	void setcolor (int r, int g, int b);
	void settransparency (int t);
	void fputpixel (int x, int y, int r, int g, int b);
	void fputpixel (int x, int y);
	void putpixel (int x, int y, int r, int g, int b);
	void putpixel (int x, int y);
	void clear (void);
	void rectangle (int x0, int y0, int x1, int y1);
	void line (int x0, int y0, int x1, int y1);
	void circle (int xc, int yc, double r);
	void putstr (int x, int y, uint32_t pixel, std::string const &texte);
    };

    int Random (int max);

} // namespace bulkrays

#endif // GRBITMAP_H_INCLUDE
