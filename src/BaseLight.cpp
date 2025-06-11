#include "BaseLight.h"

//#include "common.h"
#include "log.h"

#include <thread>
#include <cmath>

BaseLight::BaseLight(std::string const& ip, unsigned int startChannel) :
BaseItem(ip,startChannel),
    m_r(0),
    m_g(0),
    m_b(0)
{
}

BaseLight::~BaseLight() {

}

bool BaseLight::SendData( unsigned char *data) {
    try
    {
        if(m_unreachable){
            return false;
        }

        if(m_startChannel == 0){
            return false;
        }

        uint8_t r = data[m_startChannel - 1];
        uint8_t g = data[m_startChannel];
        uint8_t b = data[m_startChannel + 1];

        if(r == m_r && g == m_g && b == m_b) {
            if(m_seqCount < 1201) {
                ++ m_seqCount;
                return true;
            }
        }
        m_seqCount=0;
        m_r = r;
        m_g = g;
        m_b = b;

        std::thread t(&BaseLight::outputData, this, r, g, b );
        t.detach();
        //outputData(w );
        return true;
    }
    catch(std::exception const& ex)
    {
        m_unreachable = true;
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return false;
}

void BaseLight::outputData( uint8_t r ,uint8_t g ,uint8_t b ) {
    setLightOnRGB(r,g,b);
}

void BaseLight::RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV) {
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

    if(fH < 0.0){
        fH += 360.0;
    }
}

void BaseLight::HSVtoRGB(float H, float S, float V, float& fR, float& fG, float& fB) {
    float r, g, b;
    float h = H / 360.0f;
    float s = S / 100.0f;
    float v = V / 100.0f;

    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    fR = r * 255;
    fG = g * 255;
    fB = b * 255;
}

