#ifndef PNGMEMBUF_U_INCLUDE
#define PNGMEMBUF_U_INCLUDE

#include <iostream>

namespace bulkrays {
    int pngwrite (int w, int h, char *bitmap, std::ostream& out);
}

#endif // PNGMEMBUF_U_INCLUDE
