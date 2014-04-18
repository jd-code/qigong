
#include <errno.h>
#include <string.h>

#include <fstream>

#define QICONN_H_GLOBINST
#include "qicrypt.h"

using namespace std;
using namespace qiconn;

void usage (ostream &cout) {
    cout << "qigenkey fqdn_name" << endl
	 << "   outputs a key matching the fully qualified domain name supplied" << endl;
}

int main (int nb, char ** cmde) {

    stringstream wack;	// the computed output isn't flushed until the
			// end to prevent useless file creation if errors occur ...
    string contkey;	// concatenation of the whole key for final checksum

    if (nb != 2) {
	usage (cerr);
	return 1;
    }

    string fqdn(cmde[1]);
    string filename = fqdn + ".key.priv";

    if (fqdn.empty()) {
	usage(cerr);
	return 1;
    }

    {	ifstream fcheck(filename.c_str());
	if (fcheck) {
	    cerr << "a file named " << filename << " already exist." << endl << "cowardly refuse to overwrite it !" << endl;
	    return 1;
	}
    }

    ofstream fout (filename.c_str());
    if (!fout) {
	int e = errno;
	cerr << "could not open file " << filename << " : " << strerror(e) << endl;
	return 1;
    }

    cout << "attempting to create key " << filename << ", this may take some time ..." << endl;

    wack << "; " << fqdn << endl;

    MCRYPT td;	// only for getting iv and key sizes

    td = mcrypt_module_open((char *)"twofish", NULL, (char *)"cfb", NULL);
    if (td==MCRYPT_FAILED) {
	cerr << "could not initialise module mcrypt twofish" << endl;
	unlink (filename.c_str());
	return 1;
    }


    ifstream frandom("/dev/random");
    if (!frandom) {
	int e = errno;
	cerr << "could not open /dev/urandom : " << strerror (e) << endl;
	mcrypt_generic_end(td);
	unlink (filename.c_str());
	return 1;
    }

    size_t i;

    string key;
    size_t keysize = 32;    // 256 bits
    string iv;
    size_t ivsize = mcrypt_enc_get_iv_size(td);
    string keyid;
    size_t total = keysize + ivsize + 4;
    size_t count = 1;


    for (i=0 ; frandom && (i<keysize) ; i++) {
	key += frandom.get();
	cout << "\r" << (100*count++)/total << "% ";
	cout.flush();
    }
    if (i != keysize) {
	int e = errno;
	cerr << "could not get enough bytes from /dev/urandom for key : " << strerror (e) << endl;
	mcrypt_generic_end(td);
	unlink (filename.c_str());
	return 1;
    }

    string keyb64;
    base64_encode (key, keyb64);
    wack << keyb64 << endl;
    contkey += key;

    for (i=0 ; frandom && (i<ivsize) ; i++) {
	iv += frandom.get();
	cout << "\r" << (100*count++)/total << "% ";
	cout.flush();
    }
    if (i != ivsize) {
	int e = errno;
	cerr << "could not get enough bytes from /dev/urandom for iv : " << strerror (e) << endl;
	mcrypt_generic_end(td);
	unlink (filename.c_str());
	return 1;
    }

    string ivb64;
    base64_encode (iv, ivb64);
    wack << ivb64 << endl;
    contkey += iv;



    for (i=0 ; frandom && (i<4) ; i++) {
	keyid += frandom.get();
	cout << "\r" << (100*count++)/total << "% ";
	cout.flush();
    }
    if (i != 4) {
	int e = errno;
	cerr << "could not get enough bytes from /dev/urandom for keyid salt : " << strerror (e) << endl;
	mcrypt_generic_end(td);
	unlink (filename.c_str());
	return 1;
    }
    keyid += fqdn;

    string keyidb64;
    base64_encode (keyid, keyidb64);
    wack << keyidb64 << endl;
    contkey += keyid;

    string checksum = nekchecksum (contkey);
    string checksum64;
    base64_encode (checksum, checksum64);
    wack << checksum64 << endl;

    fout << wack.str();

    cout << "done." << endl;

    mcrypt_generic_end(td);
    return 0;
}
