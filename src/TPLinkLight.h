#pragma once

#include "TPLinkItem.h"

class TPLinkLight : public TPLinkItem{
public:
    TPLinkLight(std::string const& ip, unsigned int startChannel );
    virtual ~TPLinkLight();

    bool SendData( unsigned char *data) override;

    std::string GetType() const override {return "Light";}

    std::string setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp = 0, int period = 0);
    std::string setLightOnHSV( int hue, int saturation, int brightness, int color_Temp = 0, int period = 0);
    std::string setLightOff();

private:
    uint8_t m_r;
    uint8_t m_g;
    uint8_t m_b;

    void outputData( uint8_t r ,uint8_t g ,uint8_t b );
    void RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV);
};