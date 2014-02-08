
#ifndef HHQICRYPTINC
#define HHQICRYPTINC

#include <mcrypt.h>

#include <qiconn/qiconn.h>

namespace qiconn {

    int base64_encode (const char *s, size_t l, string &r);
    int base64_encode (const string &s, string &r);
    int base64_decode (const string &s, string &r);
    size_t base64_seekanddecode (const string &s, string &r, size_t start=0);

    class CryptConnection : public SocketConnection {
	private:
	    char *key;
	    size_t keysize;
	    char * IVin;
	    char * IVout;
	    size_t IVsize;
	    MCRYPT tdin;
	    MCRYPT tdout;
	    enum {  WaitingChallenge,
		    WaitingAnswer,
		    Refused,
		    Accepted
	    } challenging;
	    string challenge;
	protected:
	    void opencrypt(void);
	    void closecrypt(void);
	public:
	    CryptConnection (int fd, struct sockaddr_in const &client_addr, const string &crkeyb64);

	    virtual size_t read (void);
	    void prelineread (void);
//	    int ungarble (void);
	    virtual void flush (void);

	    virtual void firstprompt (void) {}

	    virtual ~CryptConnection ();
    };

}

#endif
