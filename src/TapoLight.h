#pragma once

#include "TapoItem.h"
#include "BaseLight.h"

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
#include "log.h"

class TapoLight  : public TapoItem, public BaseLight {
public:
    TapoLight(std::string const& ip, unsigned int startChannel );
    virtual ~TapoLight();

    std::string GetType() const override { return "TapoLight"; }

    bool setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp = 0, int period = 0) override;
    bool setLightOnHSV( int hue, int saturation, int brightness, int color_Temp = 0, int period = 0) override;
    bool setLightOff() override;
    
private:

};