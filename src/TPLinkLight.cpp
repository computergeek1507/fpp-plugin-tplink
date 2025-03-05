#include "TPLinkLight.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <arpa/inet.h>  // for inet_pton
#include <netinet/in.h> // for sockaddr_in, htons
#include <stdint.h>     // for uint16_t, uint8_t, uint32_t
#include <stdio.h>      // for printf
#include <sys/socket.h> // for AF_INET, connect, send, socket, SOCK_STREAM
#include <unistd.h>     // for close, read
#include <string>       // for string
#include <cstring>      // for ??? memcpy, memset, strncpy


#include <iostream>
#include <istream>
#include <ostream>

TPLinkLight::TPLinkLight(std::string const& ip, unsigned int startChannel):
 BaseItem(ip,startChannel), TPLinkItem(ip,startChannel), BaseLight(ip,startChannel)

{
}

TPLinkLight::~TPLinkLight() {

}

bool TPLinkLight::setLightOnHSV( int hue, int saturation, int brightness, int color_Temp, int period) {
    //{"smartlife.iot.smartbulb.lightingservice":{"transition_light_state":{"ignore_default":1,"transition_period":150,"mode":"normal","hue":120,"on_off":1,"saturation":65,"color_temp":0,"brightness":10}}}
    
    const std::string cmd = "{\"smartlife.iot.smartbulb.lightingservice\":{\"transition_light_state\":{\"ignore_default\":1,\"transition_period\":" + std::to_string(period) + ",\"mode\":\"normal\",\"hue\":" 
    + std::to_string(hue) + ",\"on_off\":1,\"saturation\":" + std::to_string(saturation) + ",\"color_temp\":" + std::to_string(color_Temp) + ",\"brightness\":" + std::to_string(brightness) + "}}}";
    return !sendCmd(cmd).empty();
}

bool TPLinkLight::setLightOff(){

    const std::string cmd = "{\"smartlife.iot.smartbulb.lightingservice\":{\"transition_light_state\":{\"ignore_default\":1,\"transition_period\":0,\"mode\":\"normal\",\"on_off\":0}}}";
    return !sendCmd(cmd).empty();
}

bool TPLinkLight::setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp, int period) {
    float h,si,sv,i,v;

    RGBtoHSIV(r/255,g/255,b/255,h,si,sv,i,v);
    
    int ih = (h);
    //int isi = (si*100);
    int isv = (sv*100);
    //int ii = (i*100);
    int iv = (v*100);
    return setLightOnHSV(ih, isv, iv, color_Temp, period);
}