#ifndef _MODEM_H
#define _MODEM_H

class Modem
{
public:
    Modem(Stream &str, int sleepPin = -1) {
        this->stream = &str;
        this->sleepPin=sleepPin;
    };

    virtual bool begin();
    virtual bool send(const char* msg);
    virtual bool sleep();

protected:
    Stream * stream; // Communicating with the modem
    int sleepPin;
};

#endif
