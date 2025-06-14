#pragma once

//#include "common.h"
//#include "log.h"
#include "BaseItem.h"

class BaseLight : virtual public BaseItem{
public:
    BaseLight(std::string const& ip, unsigned int startChannel  );
    virtual ~BaseLight();

    bool SendData( unsigned char *data) override;

    virtual bool setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp = 0, int period = 0) = 0;
    virtual bool setLightOnHSV( int hue, int saturation, int brightness, int color_Temp = 0, int period = 0) = 0;
    virtual bool setLightOff() = 0;

protected:
    uint8_t m_r;
    uint8_t m_g;
    uint8_t m_b;

    void outputData( uint8_t r ,uint8_t g ,uint8_t b );
    void RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV);
    void HSVtoRGB(float H, float S, float V, float& fR, float& fG, float& fB);
};