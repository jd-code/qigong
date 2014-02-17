
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <fstream>

#include "qicrypt.h"

namespace qiconn {

    using namespace std;

static bool debugcrypt = true;

    void setdebugcrypt (bool b) {
	debugcrypt = b;
    }

    ostream& operator<< (ostream& cout, CryptConnectionChallengeState const& a) {
	switch (a) {
	    case    WaitingRemoteSalt:  return cout << "WaitingRemoteSalt";
            case    WaitingHash      :  return cout << "WaitingHash";
            case    WaitingChallenge :  return cout << "WaitingChallenge";
            case    WaitingAnswer    :  return cout << "WaitingAnswer";
            case    Refused          :  return cout << "Refused";
            case    Accepted         :  return cout << "Accepted";
	}
	return cout;
    }


    int gen2sk (string &result, size_t askedsize, const string &salt1, const string &salt2, const string& data) {
	char buf[64];
	if (askedsize > 64)
	    return -1;
	keygenid algorithm = KEYGEN_S2K_SALTED;
	if (mhash_keygen_uses_salt(algorithm) != 1) // no salt, not suitable then ...
	    return -1;
	size_t salt_size = mhash_get_keygen_salt_size (algorithm);
	size_t halfsaltsize = salt_size>>1;
	if ((salt1.size() < halfsaltsize)  || (salt2.size() < halfsaltsize))
	    return -1;


	KEYGEN keygen;

	int nbalgo;
	if ((nbalgo = mhash_keygen_uses_hash_algorithm (algorithm)) != 0) {
	    for (int i=0 ; (i<nbalgo) && (i<2) ; i++) {
		keygen.hash_algorithm[i] = MHASH_MD5;
	    }
	}
	string salt(salt1, 0, halfsaltsize);
	salt.append(salt2, 0, halfsaltsize);
	keygen.salt_size = salt.size();
	keygen.salt = (void *)salt.c_str();

	if (mhash_keygen_uses_count (algorithm) == 1) {
	    keygen.count = 2;
	}

	size_t datasize = data.size();
	if (mhash_keygen_ext (algorithm, keygen, buf, askedsize, (mutils_word8 *)data.c_str(), datasize) != MUTILS_OK)
	    return -1;
    
	result.assign (buf, askedsize);
	return 0;
    }

    int getsaltforurandom (string& salt, size_t l=4) {
	size_t i;
	salt.clear();
	ifstream urandom("/dev/urandom");
	if (!urandom) {
	    return -1;
	}
	for (i=0 ; urandom && (i<l) ; i++) {
	    salt += (char)urandom.get();
	}
	if (i != l)
	    return -1;
	return 0;
    }

    static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
				    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',                 
				    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',                 
				    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',                 
				    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',                 
				    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',                 
				    'w', 'x', 'y', 'z', '0', '1', '2', '3',                 
				    '4', '5', '6', '7', '8', '9', '+', '/'};                
	
    static int decoding_table[] = {
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 
     52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6, 
      7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
     -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 
     49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    };  

    int base64_encode (const char *s, size_t l, string &r) {
	size_t i;
	
	for (i=0 ; i<l ; ) {    
	    int oa = (i<l) ? (unsigned char)(s[i++]) : 0;
	    int ob = (i<l) ? (unsigned char)(s[i++]) : 0;  
	    int oc = (i<l) ? (unsigned char)(s[i++]) : 0;  
	    int tr = (oa << 16) + (ob << 8) + oc;
	    r += encoding_table [(tr >> 18) & 0x3f];
	    r += encoding_table [(tr >> 12) & 0x3f];
	    if (i == l) {
		switch (l % 3) {
		    case 0:
			r += encoding_table [(tr >> 6) & 0x3f];                 
			r += encoding_table [ tr       & 0x3f];                 
			break;
		    case 2:
			r += encoding_table [(tr >> 6) & 0x3f];                 
			r += "=";
			break;
		    case 1:
			r += "==";
		}
	    } else {
		r += encoding_table [(tr >> 6) & 0x3f];
		r += encoding_table [ tr       & 0x3f];
	    }
	}
	return 0;
    }
    int base64_encode (const string &s, string &r) {
	return base64_encode (s.c_str(), s.size(), r);
    }


    int base64_decode (const string &s, string &r) {
	size_t i, l=s.size();
	unsigned int sa, sb, sc, sd;

	for (i=0 ; i<l ; ) {
	    size_t endingsize = 1;
	    if (decoding_table[(unsigned char)s[i]] == -1) return -1;
	    sa = decoding_table[(unsigned char)s[i++]] ;
	    if (i == l) return -1;
	    if (decoding_table[(unsigned char)s[i]] == -1) return -1;
	    sb = decoding_table[(unsigned char)s[i++]] ;
	    if (i == l) return -1;
	    if (s[i] == '=') {
		sc = 0;
		i++;
	    } else {
		if (decoding_table[(unsigned char)s[i]] == -1) return -1;
		sc = decoding_table[(unsigned char)s[i++]] ;
		endingsize ++;
	    }
	    if (i == l) return -1;
	    if (s[i] == '=') {
		sd = 0;
		i++;
	    } else {
		if (decoding_table[(unsigned char)s[i]] == -1) return -1;
		sd = decoding_table[(unsigned char)s[i++]] ;
		endingsize ++;
	    }

	    unsigned int tr = (sa << 18) + (sb << 12) + (sc << 6) + sd;
	    for (size_t j=0 ; j<endingsize ; j++) {
		r += ((tr >> ((2-j)*8)) & 0xff);
	    }
	}
	return 0;
    }

    inline bool isbase64 (int n) {
	return (decoding_table[n & 0xff] != -1);
    }

    size_t base64_seekanddecode (const string &s, string &r, size_t start /* =0 */) {
	size_t p=start, q;
	while (isspace(s[p])) p++;
	q = p;
	while (isbase64(s[q]) || (s[q] == '=')) q++;
	if (base64_decode (s.substr(p,q-p), r) == -1)
	    return string::npos;
	return q;
    }

    QiCrKey::QiCrKey (const char *fname) :
	key(NULL),
	keysize(0),
	IV(NULL),
	IVsize(0),
	valid(false)
    {
	ifstream fkey(fname);
	if (!fkey) {
	    int e=errno;
	    cerr << "QiCrKey : could not read key from file " << fname << " : " << strerror(e) << endl;
	    valid = false;
	    return;
	}
	string rawkey;
	while (fkey && !fkey.eof()) {
	    string line;
	    char c;
	    while (fkey && !fkey.eof()) {
		c = fkey.get();
		
		if (fkey.eof() || (c==10) || (c==13)) break;
		line += c;
	    }
	    size_t i=0;   
	    while (i < line.size()) {
		while ((i<line.size()) && isspace(line[i])) i++;
		if (i == line.size()) break; // empty line
		if ((line[i]=='#') || (line[i] == ';')) break;	// skip comments
		while ((i<line.size()) && (isbase64(line[i]) || line[i]=='=')) rawkey += line[i++];
		rawkey += ' ';
	    }
	}
if(debugcrypt) cerr << "rawkey = [" << rawkey << "]" << endl;
	size_t p;
	string t;
	if ((p = base64_seekanddecode (rawkey, t)) == string::npos) {
	    cerr << " QiCrKey(" << fname << ") : error at decoding base64 key" << endl;
	    valid = false;
	    return;
	}
	keysize = t.size();
	key = (char *) malloc (keysize);
	if (key == NULL) {
	    cerr << "QiCrKey(" << fname << ") : could not allocate key !" << endl;
	    valid = false;
	    return;
	}
	memcpy ((void *)key, t.c_str(), keysize);

	t.clear();
	if ((p = base64_seekanddecode (rawkey, t, p)) == string::npos) {
	    cerr << "QiCrKey(" << fname << ") : error at decoding base64 key (IV)" << endl;
	    valid = false;
	    return;
	}

	IVsize = t.size();
	IV = (char *) malloc (IVsize);
	if (IV == NULL) {
	    cerr << "QiCrKey(" << fname << ") : could not allocate key (IV) !" << endl;
	    valid = false;
	    return;
	}
	memcpy ((void *)IV, t.c_str(), IVsize);

	t.clear();
	if ((p = base64_seekanddecode (rawkey, t, p)) == string::npos) {
	    cerr << "QiCrKey(" << fname << ") : error at decoding base64 key (keyID)" << endl;
	    valid = false;
	    return;
	}
	keyID.assign(t);
	valid = true;
    }

    QiCrKey::~QiCrKey (void) {
	valid = false;
	if (key != NULL) free ((void *)key);
	if (IV != NULL) free ((void *)IV);
    }

    string QiCrKey::getReadableID(void) const {
	string r(keyID.substr(4));
	size_t i;
static const char *digit = "0123456789abcdef";
	for (i=0 ; (i<keyID.size()) && (i<4) ; i++) {
	    r += digit[((int)keyID[i] >> 4) & 0x0f];
	    r += digit[((int)keyID[i]     ) & 0x0f];
	}
	return r;
    }

    KeyRing::KeyRing () {}

    KeyRing::~KeyRing (void) {
	map<string,QiCrKey *>::iterator mi;
	for (mi=mkeys.begin() ; mi!=mkeys.end() ; mi++)
	    delete (mi->second);
    }

    void KeyRing::addkey (const char * fname) {
	QiCrKey *nkey = new QiCrKey(fname);
	if (nkey == NULL) {
	    cerr << "KeyRing::addkey (" << fname << ") could not allocate QiCrKey" << endl;
	    return;
	}
	if (!nkey->isvalid()) {
	    delete (nkey);
	    return;
	}
	map<string,QiCrKey *>::iterator mi = mkeys.find(nkey->getfullkeyID());
	if (mi != mkeys.end()) {
	    cerr << "KeyRing::addkey (" << fname << ") a key with ID=" << nkey->getReadableID()
		 << " is a lready in keyring, skipped" << endl;
	    delete (nkey);
	    return;
	}
if(debugcrypt) cerr << "KeyRing::addkey += " << nkey->getReadableID () << endl;;
	mkeys[nkey->getfullkeyID()] = nkey;
    }

    void KeyRing::setwalletdir (const char *walletdir) {
	KeyRing::walletdir = walletdir;
    }

    const QiCrKey * KeyRing::gentlyaddkey (const string &fname) {
	string seekname;
	if (walletdir.empty() || (fname[0]=='/'))
	    seekname.append(fname);
	else {
	    seekname.append(walletdir);
	    seekname.append("/");
	    seekname.append(fname);
	}
	seekname.append(".key.priv");
	
	QiCrKey *nkey = new QiCrKey(seekname.c_str());
	if (nkey == NULL) {
	    cerr << "KeyRing::gentlyaddkey (" << seekname << ") could not allocate QiCrKey" << endl;
	    return NULL;
	}
	if (!nkey->isvalid()) {
	    delete (nkey);
	    return NULL;
	}
	map<string,QiCrKey *>::iterator mi = mkeys.find(nkey->getfullkeyID());
	if (mi != mkeys.end()) {
	    delete (nkey);
	    return mi->second;
	} else {
	    mkeys[nkey->getfullkeyID()] = nkey;
if(debugcrypt) cerr << "KeyRing::gentlyaddkey += " << nkey->getReadableID () << endl;;
	    return nkey;
	}
    }

    QiCrKey * KeyRing::retreivekey (const string &salt1, const string &salt2, const string &hash) const {
	map<string,QiCrKey *>::const_iterator mi;
	for (mi=mkeys.begin() ; mi!=mkeys.end() ; mi++) {
	    string lhash;
	    if (gen2sk (lhash, 8, salt1, salt2, mi->second->getfullkeyID()) != 0) continue;   // on error skip
if(debugcrypt) cerr << "KeyRing::retreivekey " << hexdump(lhash) 
     << "                     " << hexdump(hash) << endl;
	    if (lhash == hash)
		return mi->second;
	}
	return NULL;
    }

    void QiCrKey::getkey (const char * &k, size_t &ks) const {
	k = key, ks = keysize;
    }

    void QiCrKey::getIV (const char * &iv, size_t &ivs) const {
	iv = IV, ivs = IVsize;
    }

    void CryptConnection::settlekey (QiCrKey const* qicrkey) {
	if (qicrkey == NULL) {
	    cerr << gettype() << "::" << getname() << " settlekey called with NULL" << endl;
	    return;
	}
	CryptConnection::qicrkey = qicrkey;
	qicrkey->getkey(key, keysize);
	qicrkey->getIV(IVin, IVsize);
	qicrkey->getIV(IVout, IVsize);
	keyID = qicrkey->getfullkeyID ();
if (debugcrypt) cerr << gettype() << "::" << getname() << " settlekey " << qicrkey->getReadableID () << endl;
    }

    CryptConnection::CryptConnection (int fd, struct sockaddr_in const &client_addr, const KeyRing* keyring) : 
	SocketConnection (fd, client_addr),
	      key (NULL),
	  keysize (0),
	     IVin (NULL),
	    IVout (NULL),
	   IVsize (0),
	     tdin (MCRYPT_FAILED),
	    tdout (MCRYPT_FAILED),
      challenging (WaitingRemoteSalt),
	  keyring (keyring),
	  qicrkey (NULL),
      issuplicant (false)
    {
	if (fd >= 0)
	    opencrypt();
    }

    CryptConnection::CryptConnection (int fd, struct sockaddr_in const &client_addr, const QiCrKey* qicrkey) : 
	SocketConnection (fd, client_addr),
	      key (NULL),
	  keysize (0),
	     IVin (NULL),
	    IVout (NULL),
	   IVsize (0),
	     tdin (MCRYPT_FAILED),
	    tdout (MCRYPT_FAILED),
      challenging (WaitingRemoteSalt),
	  keyring (NULL),
	  qicrkey (qicrkey),
      issuplicant (true)
    {
//	string t;
//	size_t p;
//	if ((p = base64_seekanddecode (crkeyb64, t)) == string::npos) {
//	    cerr << "CryptConnection : error at decoding base64 key" << endl;
//	    flushandclose();
//	    return;
//	}
//
//	keysize = t.size();
//	key = (char *) malloc (keysize);
//	if (key == NULL) {
//	    cerr << "CryptConnection : could not allocate key !" << endl;
//	    flushandclose();
//	    return;
//	}
//	memcpy (key, t.c_str(), keysize);
//
//	t.clear();
//	if ((p = base64_seekanddecode (crkeyb64, t, p)) == string::npos) {
//	    cerr << "CryptConnection : error at decoding base64 key (IV)" << endl;
//	    flushandclose();
//	    return;
//	}
//
//	IVsize = t.size();
//	IVin = (char *) malloc (IVsize);
//	if (IVin == NULL) {
//	    cerr << "CryptConnection : could not allocate key (IVin) !" << endl;
//	    flushandclose();
//	    return;
//	}
//	memcpy (IVin, t.c_str(), IVsize);
//	IVout = (char *) malloc (IVsize);
//	if (IVout == NULL) {
//	    cerr << "CryptConnection : could not allocate key (IVout) !" << endl;
//	    flushandclose();
//	    return;
//	}
//	memcpy (IVout, t.c_str(), IVsize);

	// this below can easily be "overloaded" at need by derivated constructors
	if (qicrkey != NULL) {
	    keyID.assign(qicrkey->getfullkeyID());
if(debugcrypt) cerr << "CryptConnection with " << qicrkey->getReadableID () << endl;
	}

	if (fd >= 0)
	    opencrypt();
    }

    void CryptConnection::reallycrypt(void) {
	tdin = mcrypt_module_open((char *)"twofish", NULL, (char *)"cfb", NULL);
	if (tdin==MCRYPT_FAILED) {
	    cerr << "CryptConnection : could not init tdin mcrypt module" << endl;
	    flushandclose();
	    return;
	}
	if (IVsize != (size_t) mcrypt_enc_get_iv_size(tdin)) {
	    cerr << "CryptConnection : IV size mismatch : " << IVsize << " != " << mcrypt_enc_get_iv_size(tdin) << endl;
	    mcrypt_generic_end(tdin);
	    tdin = MCRYPT_FAILED;
	    flushandclose();
	    return;
	}
	if (mcrypt_generic_init( tdin, (void*)key, keysize, (void*)IVin) < 0) {
	    cerr << "CryptConnection : error at tdin init" << endl;
	    mcrypt_generic_end(tdin);
	    tdin = MCRYPT_FAILED;
	    flushandclose();
	    return;
	}
	
	tdout = mcrypt_module_open((char *)"twofish", NULL, (char *)"cfb", NULL);
	if (tdout==MCRYPT_FAILED) {
	    cerr << "CryptConnection : could not init tdout mcrypt module" << endl;
	    flushandclose();
	    return;
	}
	if (IVsize != (size_t) mcrypt_enc_get_iv_size(tdout)) {
	    cerr << "CryptConnection : IV size mismatch : " << IVsize << " != " << mcrypt_enc_get_iv_size(tdout) << endl;
	    mcrypt_generic_end(tdout);
	    tdout = MCRYPT_FAILED;
	    flushandclose();
	    return;
	}
	if (mcrypt_generic_init( tdout, (void*)key, keysize, (void*)IVout) < 0) {
	    cerr << "CryptConnection : error at tdout init" << endl;
	    mcrypt_generic_end(tdout);
	    tdout = MCRYPT_FAILED;
	    flushandclose();
	    return;
	}

	    size_t i;
	    challenge.clear();
	    ifstream urandom("/dev/urandom");
	    if (!urandom) {
		int e = errno;
		cerr << "CryptConnection : could not open /dev/urandom : " << strerror (e) << endl;
		flushandclose();
		return;
	    }
	    for (i=0 ; i<64 ; i++) {
		challenge += (char)urandom.get();
	    }
if(debugcrypt) cerr << gettype() << "::" << getname() << " challenge=" << hexdump(challenge) << endl;
	    (*out) << challenge;
	    flush();
    }

    void CryptConnection::opencrypt(void) {
	if (getsaltforurandom (salt1, 4) != 0) {
	    int e = errno;
	    cerr << "CryptConnection : could not salt1 from /dev/urandom : " << strerror (e) << endl;
	    challenging = Refused;
	    flushandclose();
	}
	(*out) << salt1;
if(debugcrypt) cerr << "salt1=" << hexdump(salt1) << endl;
	challenging = WaitingRemoteSalt;
	flush ();
    }

    void CryptConnection::closecrypt(void) {
	if (tdout != MCRYPT_FAILED)
	    mcrypt_generic_end(tdout);
	tdout = MCRYPT_FAILED;

	if (tdin != MCRYPT_FAILED)
	    mcrypt_generic_end(tdin);
	tdin = MCRYPT_FAILED;

    }

    CryptConnection::~CryptConnection (void) {
	closecrypt();

	// if (IVout != NULL) free (IVout);
	IVout = NULL;

	// if (IVin != NULL) free (IVin);
	IVin = NULL;

	IVsize = 0;

	// if (key != NULL) free (key);	
	key = NULL;

	keysize = 0;
    }

inline size_t min (size_t a, size_t b) { return a<b ? a : b; }


    #define BUFLEN 32768
    size_t CryptConnection::read (void) {
	char s[BUFLEN];
	ssize_t n = ::read (fd, (void *)s, BUFLEN);
	
	if (n == -1) {
	    int e = errno;
	    cerr << "read(" << fd << ") error : " << strerror (e) << endl;
	    return 0;
	}

	if (debug_transmit) {
	    int i;
	    clog << "fd=" << fd << "; ";
	    for (i=0 ; i<n ; i++)
		clog << s[i];
	    clog << endl;
	}
if (debug_dummyin)
{   int i;
    cerr << "BufConnection::read got[";
    for (i=0 ; i<n ; i++)
	cerr << s[i];
    cerr << endl;
}
	int i;
	for (i=0 ; i<n ; i++) {
	    if (!raw) {
		if ((s[i]==10) || (s[i]==13) || s[i]==0) {
		    if (i+1<n) {
			if ( ((s[i]==10) && (s[i+1]==13)) || ((s[i]==13) && (s[i+1]==10)) )
			    i++;
		    }
if (debug_lineread) {
    cerr << "BufConnection::read->lineread(" << bufin << ")" << endl;
}
		    prelineread ();
		    bufin.clear();
		} else
		    bufin += s[i];
	    } else {
		bufin += s[i];
	    }
	}
	if (raw) {
if (debug_lineread) {
    cerr << "BufConnection::read->lineread(" << bufin << ")" << endl;
}
	    prelineread ();
	    bufin.clear();
	} else if ((maxpendsize != string::npos) && (bufin.size() > maxpendsize)) {
	    cerr << "BufConnection::read " << gettype() << "::" << getname() << " bufin.size=" << bufin.size() << " > " << maxpendsize << " : closing connection" << endl;
	    schedule_for_destruction();
	}
	if (n==0) {
	    if (debug_newconnect) cerr << "read() returned 0. we may close the fd[" << fd << "] ????" << endl;
	    reconnect_hook();
	}
	return (size_t) n;
    }

    void CryptConnection::prelineread (void) {
	int r = 0;
	string temp;
	if (base64_decode (bufin, temp) != 0) r++;
	bufin.clear();

	if (tdin != MCRYPT_FAILED) {
	    char buf[2048];
	    size_t p;
	    size_t s = temp.size();

	    for (p=0 ; p<s ; p+= 2048) {
		size_t bs = min(s-p,2048);
		memcpy (buf, temp.c_str()+p, bs);
		if (mdecrypt_generic (tdin, (void *)buf, bs) != 0)
		    r++;
		else
		    temp.assign (buf,bs);
	    }
	} else if ((challenging != WaitingRemoteSalt) && (challenging != WaitingHash)) {
cerr << "CryptConnection::prelineread " << gettype() << "::" << getname() << " skipping lineread datas because of strange condition : "  << challenging << "!" << endl;
	    return;
	}

	if (r!=0) { // JDJDJDJD this is rather strict and prevails over the application ...
	    cerr << "CryptConnection::ungarble : too much garbling, closing " << gettype() << " : " << getname() << endl;
	    flushandclose();	    // JDJDJDJD which one is the best ?
	    schedule_for_destruction ();
	    return;
	}

	if (challenging != Accepted) {
	    switch (challenging) {
		case WaitingRemoteSalt: {
		    string &salt2 = temp;
if(debugcrypt) cerr << gettype() << "::" << getname() << " salt2=" << hexdump(salt2) << endl;
		    string salt3;
		    if (getsaltforurandom (salt3, 4) != 0) {
			int e = errno;
			cerr << gettype() << "::" << getname() << " could not salt3 from /dev/urandom : " << strerror (e) << endl;
			challenging = Refused;
			flushandclose();
		    } else {
			string hash;
			const string &hashdata = keyID;
			if (gen2sk (hash, 8, salt2, salt3, hashdata) != 0) {
cerr << gettype() << "::" << getname() << " gen2sk failed : closing" << endl;
			    challenging = Refused;
			    flushandclose();
			} else {
if(debugcrypt) cerr << gettype() << "::" << getname() << " salt3=" << hexdump(salt3) << endl;
			    (*out) << salt3;
if(debugcrypt) cerr << gettype() << "::" << getname() << " hashout=" << hexdump(hash) << endl;
			    (*out) << hash;
			    flush();
if(debugcrypt) cerr << gettype() << "::" << getname() << " challenging = WaitingHash" << endl;
			    challenging = WaitingHash;
			}
		    }
		  }
		  return;

		case WaitingHash:
		    {	string salt4(temp,0,4);
if(debugcrypt) cerr << gettype() << "::" << getname() << " salt4=" << hexdump(salt4) << endl;
			string hash(temp,4);
if(debugcrypt) cerr << gettype() << "::" << getname() << " hash_in=" << hexdump(hash) << endl;
			if ((!issuplicant) && (keyring != NULL)) {
			    QiCrKey * matchkey = keyring->retreivekey (salt1, salt4, hash);
			    if (matchkey == NULL) {
				cerr << gettype() << "::" << getname() << " could not find matching key in keyring : closing" << endl;
				challenging = Refused;
				flushandclose();
			    } else {
				settlekey (matchkey);
if(debugcrypt) cerr << gettype() << "::" << getname() << " challenging = WaitingChallenge" << endl;
				reallycrypt();
				challenging = WaitingChallenge;
			    }
			} else { // yet no verification on hash on supllicant side ...
			    if (qicrkey != NULL) {
				settlekey (qicrkey);
if(debugcrypt) cerr << gettype() << "::" << getname() << " challenging = WaitingChallenge [2]" << endl;
				reallycrypt();
				challenging = WaitingChallenge;
			    } else {
				cerr << gettype() << "::" << getname() << " we're supplicant side and don't have a key : closing" << endl;
				challenging = Refused;
				flushandclose();
			    }
			}
		    }
		    return;

		case WaitingChallenge:
if(debugcrypt) cerr << gettype() << "::" << getname() << " challenge=" << hexdump(temp) << endl;
		    if (temp == challenge) {
			challenging = Refused;
cerr << gettype() << "::" << getname() << " got mirror challenge : closing" << endl;
			flushandclose();
		    } else {
			(*out) << temp;
			flush();
			challenging = WaitingAnswer;
		    }
		    return;
		case WaitingAnswer:
		    r = -1;
		    if (temp == challenge) {
			challenging = Accepted;
			firstprompt ();
		    } else {
			challenging = Refused;
cerr << gettype() << "::" << getname() << " challenge mismatch : closing" << endl;
			flushandclose();
		    }
		    return;
		case Refused:
		    flushandclose();
		    return;
		case Accepted:	// Oh well ...
		    break;
	    }
	}

	{
static string matcheol("\012\015\000", 3);
	    size_t q = temp.find_first_of(matcheol);;
	    if (q == string::npos) {
if (debug_lineread) {
    cerr << "CryptConnection::prelineread->lineread(" << bufin << ")" << endl;
}
		bufin.swap (temp);
		lineread();
		bufin.clear();
		return;
	    }

	    size_t l=temp.size(), p=0;
	    while (q != string::npos) {
		bufin = temp.substr(p,q-p);
if (debug_lineread) {
    cerr << "CryptConnection::prelineread->lineread(" << bufin << ")" << endl;
}
		lineread();
		bufin.clear();
		if (q+1 < l) {
		    if ( ((temp[q]==10) && (temp[q+1]==13)) ||
			 ((temp[q]==13) && (temp[q+1]==10)) )
			q++;
		}
		p = q+1;
		if (p>=l) return;
		q = temp.find_first_of(matcheol, p);
	    }

	    bufin = temp.substr(p);
if (debug_lineread) {
    cerr << "CryptConnection::prelineread->lineread(" << bufin << ")" << endl;
}
	    lineread();
	    bufin.clear();
	}
    }

    void CryptConnection::flush (void) {
	if (tdout != MCRYPT_FAILED) {
	    size_t s = out->str().size();
	    char buf[2048];

	    size_t p;

	    for (p=0 ; p<s ; p+=2048) {
		size_t bs = min (s-p, 2048);
		memcpy (buf, out->str().c_str()+p, bs);
		mcrypt_generic (tdout, (void *)buf, bs);
		base64_encode (buf,bs, bufout);
	    }

	    bufout += 10;
	    bufout += 13;
	} else if ((challenging == WaitingRemoteSalt) || (challenging == WaitingHash)) {
	    base64_encode (out->str(), bufout);

	    bufout += 10;
	    bufout += 13;
	}
	if (cp != NULL)
	    cp->reqw (fd);
	delete (out);
	out = new stringstream ();
    }
}
