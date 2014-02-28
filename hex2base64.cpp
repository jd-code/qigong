
#define QICONN_H_GLOBINST
#include "qicrypt.h"

using namespace std;
using namespace qiconn;

int tohex (char c) {
    switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    return c-'0';
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	    return 10+c-'a';
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	    return 10+c-'A';
	default:
	    return 0;
    }
}

int appendhextos (const string &h, string &s) {
    size_t i, l = h.size();
    for (i=0 ; i<l ; ) {
	if (isspace(h[i])) {
	    i++; continue;
	}
	if (isxdigit(h[i]) && (i+1 < l) && isxdigit(h[i+1])) {
	    s += (char) ((tohex(h[i])<<4) + tohex(h[i+1]));
	    i+=2;
	} else
	    return -1;
    }
    return 0;
}

int main (int nb, char ** cmde) {
    string s;
    string out;
    if (nb > 1) {
	int i;
	for (i=1 ; i<nb ; i ++)
	    appendhextos (cmde[i], s);
    }
    base64_encode (s, out);
    cout << out << endl;
}
