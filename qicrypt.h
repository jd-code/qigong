
#ifndef HHQICRYPTINC
#define HHQICRYPTINC

#include <mcrypt.h>
#include <mhash.h>

#include <qiconn/qiconn.h>

namespace qiconn {

    void setdebugcrypt (bool b);

    class QiCrKey {
	private:
	    const char* key;
	    size_t keysize;
	    const char * IV;
	    size_t IVsize;
	    string keyID;
	    bool valid;
	public:
	    QiCrKey (const char * fname);
	    ~QiCrKey (void);
	    bool isvalid (void) { return valid; }
	    const string& getfullkeyID(void) const { return keyID; }
	    string getReadableID(void) const;
	    void getkey (const char * &k, size_t &ks) const;
	    void getIV (const char * &iv, size_t &ivs) const;
    };

    class KeyRing {
	private:
	    map<string, QiCrKey *> mkeys;
	    string walletdir;
	public:
	    KeyRing ();
	    bool addkey (const char * fname);
	    void setwalletdir (const char *walletdir);
	    const QiCrKey * gentlyaddkey (const string &fname);
	    ~KeyRing ();
	    QiCrKey * retreivekey (const string &salt1, const string &salt2, const string &hash) const;
    };

    int gen2sk (string &result, size_t askedsize, const string &salt1, const string &salt2, const string& data);
    int base64_encode (const char *s, size_t l, string &r);
    int base64_encode (const string &s, string &r);
    int base64_decode (const string &s, string &r);
    size_t base64_seekanddecode (const string &s, string &r, size_t start=0);
    string nekchecksum (const string & s);

    typedef enum {  WaitingRemoteSalt,
		    WaitingHash,
		    WaitingChallenge,
		    WaitingAnswer,
		    Refused,
		    Accepted
	    } CryptConnectionChallengeState;

    class CryptConnection : public SocketConnection {
	private:
	    const char *key;
	    size_t keysize;
	    const char * IVin;
	    const char * IVout;
	    size_t IVsize;
	    string keyID;
	    MCRYPT tdin;
	    MCRYPT tdout;
	    CryptConnectionChallengeState challenging;
	    string challenge;
	    string salt1;
	    const KeyRing* keyring;
	    const QiCrKey* qicrkey;
	    bool issuplicant;
	    void reallycrypt(void);
	protected:
	    void opencrypt(void);
	    void closecrypt(void);
	public:
	    // CryptConnection (int fd, struct sockaddr_in const &client_addr, const string &crkeyb64);
	    CryptConnection (int fd, struct sockaddr_in const &client_addr, const KeyRing* keyring);
	    CryptConnection (int fd, struct sockaddr_in const &client_addr, const QiCrKey* qicrkey);

	    void settlekey (QiCrKey const* qicrKey);

	    virtual size_t read (void);
	    void prelineread (void);
//	    int ungarble (void);
	    virtual void flush (void);

	    virtual void firstprompt (void) {}

	    virtual ~CryptConnection ();
    };

}

#endif
