
#include <stdlib.h> // malloc

#define PNG_DEBUG 3
#include <png.h>

#include "pngmembuf.h"

namespace bulkrays {

    using namespace std;

    typedef struct str_ostream_encode {
	ostream *pout;
    } OstreamEncode;

    void my_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
    {
	OstreamEncode* poe = (OstreamEncode*) png_get_io_ptr (png_ptr); /* was png_ptr->io_ptr */
	poe->pout->write ((char *)data, (streamsize) length);
    }


    int pngwrite (int w, int h, char *bitmap, ostream& out) {

        png_structp png_ptr;                                                                 
        png_infop info_ptr;                                                                  

	/* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
	    cerr << "pngwrite : error could not allocate png_struct" << endl;
	    return -1;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
	    cerr << "pngwrite : error could not allocate png_info" << endl;
	    png_destroy_write_struct (&png_ptr, &info_ptr);
	    return -1;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
            cerr << "pngwrite : Error during init_io" << endl;
	    png_destroy_write_struct (&png_ptr, &info_ptr);
	    return -1;
        }

	// png_init_io(png_ptr, fp);

	OstreamEncode oe;
	oe.pout = &out;

	png_set_write_fn(png_ptr, &oe, my_png_write_data, NULL);


	/* write header */
        if (setjmp(png_jmpbuf(png_ptr))) {
            cerr << "pngwrite : Error during writing header" << endl;
	    png_destroy_write_struct (&png_ptr, &info_ptr);
	    return -1;
        }

        png_set_IHDR(png_ptr, info_ptr, w, h,
                     8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);

	        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr))) {
            cerr << "[write_png_file] Error during writing bytes" << endl;
	    png_destroy_write_struct (&png_ptr, &info_ptr);
	    return -1;
        }

	png_bytep * row_pointers = (png_bytep *) malloc (h * sizeof(png_bytep));
	if (row_pointers == NULL) {
	    cerr << "pngwrite : could not allocate row_pointers" << endl;
	    png_destroy_write_struct (&png_ptr, &info_ptr);
	    return -1;
	}

	int y;
	for (y=0; y<h; y++) {
	    row_pointers[y] = (png_bytep) (bitmap + y * (4*w));
	}

        png_write_image(png_ptr, row_pointers);

	free (row_pointers);

        /* end write */
        if (setjmp(png_jmpbuf(png_ptr))) {
            cerr << "[write_png_file] Error during end of write" << endl;
	    png_destroy_write_struct (&png_ptr, &info_ptr);
	    return -1;
        }

        png_write_end(png_ptr, NULL);

	png_destroy_write_struct (&png_ptr, &info_ptr);
	return 0;
    }


} // namespace bulkrays


