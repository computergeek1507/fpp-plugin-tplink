#include "TapoLight.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <iostream>
#include <istream>
#include <ostream>

TapoLight::TapoLight(std::string const& ip, unsigned int startChannel ):
    BaseItem(ip,startChannel), TapoItem(ip,startChannel), BaseLight(ip,startChannel)
{ 
}

TapoLight::~TapoLight() {

}

bool TapoLight::setLightOnHSV( int hue, int saturation, int brightness, int color_Temp, int period) {
    //{"smartlife.iot.smartbulb.lightingservice":{"transition_light_state":{"ignore_default":1,"transition_period":150,"mode":"normal","hue":120,"on_off":1,"saturation":65,"color_temp":0,"brightness":10}}}
    
    const std::string cmd = "hsv h " 
    + std::to_string(hue) + " s " + std::to_string(saturation) + " v " + std::to_string(color_Temp) + " brightness " + std::to_string(brightness) + " --transition " + std::to_string(period);
    return !sendCmd(cmd).empty();
}

bool TapoLight::setLightOff(){

    const std::string cmd = "off";
    sendCmd(cmd);
    return true;
}

bool TapoLight::setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp, int period) {
    float h,si,sv,i,v;

    RGBtoHSIV(r/255,g/255,b/255,h,si,sv,i,v);
    
    int ih = (h);
    //int isi = (si*100);
    int isv = (sv*100);
    //int ii = (i*100);
    int iv = (v*100);
    return setLightOnHSV(ih, isv, iv, color_Temp, period);
}



