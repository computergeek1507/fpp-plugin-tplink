#include "TPLinkItem.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <iostream>
#include <istream>
#include <ostream>

TPLinkItem::TPLinkItem(std::string const& ip, unsigned int startChannel) :
    _startChannel(startChannel),
    _ipAddress(ip),
    _r(0),
    _g(0),
    _b(0),
    _unreachable(false),
    _seqCount(0),
    m_curl(NULL)
{
    m_curl = curl_easy_init();
}

TPLinkItem::~TPLinkItem()
{
    if (m_curl)
        curl_easy_cleanup(m_curl);
}

bool TPLinkItem::SendData( unsigned char *data)
{
    try
    {
        if(_unreachable)
            return false;

        uint8_t r = data[_startChannel - 1];
        uint8_t g = data[_startChannel];
        uint8_t b = data[_startChannel + 1];

        if(r == _r && g == _g && b == _b) {
            if(_seqCount<1201) {
                ++_seqCount;
                return true;
            }
        }
        _seqCount=0;
        _r = r;
        _g = g;
        _b = b;

        std::thread t(&TPLinkItem::outputData, this, r, g, b );
        t.detach();
        //outputData(r, g, b );
        return true;
    }
    catch(std::exception ex)
    {
        _unreachable = true;
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return false;
}

void TPLinkItem::outputData( uint8_t r ,uint8_t g ,uint8_t b )
{
    std::string newURL = _url;
    std::string newMessage = _message;

    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, _type.c_str());
    struct curl_slist *hs = NULL;
    std::string const content = "Content-Type: " + _contentType;
    hs = curl_slist_append(hs, content.c_str());
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, hs);

    if(!newMessage.empty()){
        LogInfo(VB_PLUGIN, "Data '%s'\n",newMessage.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, newMessage.c_str());
    }

    std::string const repURL = "http://" + _ipAddress + ":" + std::to_string(_port) + newURL;
    LogInfo(VB_PLUGIN, "URL '%s'\n",repURL.c_str());
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_URL, repURL.c_str());

    CURLcode status = curl_easy_perform(m_curl);
    if (status != CURLE_OK) {
        _unreachable = true;
        LogInfo(VB_PLUGIN, "failed to send curl command\n");
    }
}

void TPLinkItem::RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV) {
    float M  = std::max(std::max(fR, fG), fB);
    float m = std::min(std::min(fR, fG), fB);
    float c = M-m;
    fV = M;
    //fL = (1.0/2.0)*(M+m);
    fI = (1.0/3.0)*(fR+fG+fB);
  
    if(c==0) {
        fH = 0.0;
        fSI = 0.0;
    }
    else {
        if(M==fR) {
            fH = fmod(((fG-fB)/c), 6.0);
        }
        else if(M==fG) {
            fH = (fB-fR)/c + 2.0;
        }
        else if(M==fB) {
            fH = (fR-fG)/c + 4.0;
        }
        fH *=60.0;
        if(fI!=0) {
            fSI = 1.0 - (m/fI);
        }
        else {
            fSI = 0.0;
        }
    }

    fSV = M == 0 ? 0 : (M - m) / M;

    if(fH < 0.0)
        fH += 360.0;
}

