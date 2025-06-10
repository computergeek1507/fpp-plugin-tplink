#pragma once

#include "BaseLight.h"

class GoveeLight : public BaseLight {
public:
    GoveeLight(std::string const& ip, unsigned int startChannel );
    virtual ~GoveeLight();

    //bool SendData( unsigned char *data) override;

    std::string GetType() const override { return "GoveeLight"; }

    bool setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp = 0, int period = 0);
    bool setLightOnHSV( int hue, int saturation, int brightness, int color_Temp = 0, int period = 0);
    bool setLightOff();

private:
    //void outputData( uint8_t r ,uint8_t g ,uint8_t b );
    //void RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV);
    //uint16_t m_port{4003};
    std::string sendCmd(std::string cmd);
};