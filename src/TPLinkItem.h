#pragma once

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

#include "common.h"
#include "log.h"

#include <curl/curl.h>

class TPLinkItem {
public:
    TPLinkItem(std::string const& ip, unsigned int startChannel );
    ~TPLinkItem();

    bool SendData( unsigned char *data);

    std::string GetIPAddress(){return _ipAddress;}
    unsigned int GetStartChannel(){return _startChannel;}

    void EnableOutput(){_unreachable = false;}

private:
    std::string _ipAddress;
    unsigned int _startChannel;
    unsigned int _seqCount;

    uint8_t _r;
    uint8_t _g;
    uint8_t _b;
    std::atomic<bool> _unreachable;

    CURL *m_curl;

    void outputData( uint8_t r ,uint8_t g ,uint8_t b );

    void RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV);
};