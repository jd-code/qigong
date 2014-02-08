
#include <errno.h>
#include <string.h>

#include <fstream>

#define QICONN_H_GLOBINST
#include "qicrypt.h"

using namespace std;
using namespace qiconn;

int main (int nb, char ** cmde) {


    MCRYPT td;	// only for getting iv and key sizes

    td = mcrypt_module_open((char *)"twofish", NULL, (char *)"cfb", NULL);
    if (td==MCRYPT_FAILED) {
	cerr << "could not initialise module mcrypt twofish" << endl;
	return 1;
    }


    ifstream frandom("/dev/random");
    if (!frandom) {
	int e = errno;
	cerr << "could not open /dev/urandom : " << strerror (e) << endl;
	mcrypt_generic_end(td);
	return 1;
    }

    size_t i;

    string key;
    size_t keysize = 32;    // 256 bits
    for (i=0 ; frandom && (i<keysize) ; i++)
	key += frandom.get();
    if (i != keysize) {
	int e = errno;
	cerr << "could not get enough bytes from /dev/urandom for key : " << strerror (e) << endl;
	mcrypt_generic_end(td);
	return 1;
    }

    string keyb64;
    base64_encode (key, keyb64);
    cout << keyb64 << endl;

    string iv;
    size_t ivsize = mcrypt_enc_get_iv_size(td);
    for (i=0 ; frandom && (i<ivsize) ; i++)
	iv += frandom.get();
    if (i != ivsize) {
	int e = errno;
	cerr << "could not get enough bytes from /dev/urandom for iv : " << strerror (e) << endl;
	mcrypt_generic_end(td);
	return 1;
    }

    string ivb64;
    base64_encode (iv, ivb64);
    cout << ivb64 << endl;

    return 0;
}
