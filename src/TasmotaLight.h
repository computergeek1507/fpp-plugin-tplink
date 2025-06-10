#pragma once

#include "BaseLight.h"

#include <curl/curl.h>

class TasmotaLight : public BaseLight{
public:
    TasmotaLight(std::string const& ip, unsigned int startChannel );
    virtual ~TasmotaLight();

    //bool SendData( unsigned char *data) override;

    std::string GetType() const override { return "TasmotaLight"; }

    bool setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp = 0, int period = 0) override;
    bool setLightOnHSV( int hue, int saturation, int brightness, int color_Temp = 0, int period = 0) override;
    bool setLightOff() override;

private:
    CURL *m_curl;
    //void outputData( uint8_t r ,uint8_t g ,uint8_t b );
    //void RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV);
};