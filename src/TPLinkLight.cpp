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

TPLinkLight::TPLinkLight(std::string const& ip, unsigned int startChannel) :
    TPLinkItem(ip,startChannel),
    m_r(0),
    m_g(0),
    m_b(0)
{
}

TPLinkLight::~TPLinkLight() {

}

std::string TPLinkLight::GetConfigString() const {
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) + " Device Type: " + GetType();
}

bool TPLinkLight::SendData( unsigned char *data) {
    try
    {
        if(m_unreachable){
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

        std::thread t(&TPLinkLight::outputData, this, r, g, b );
        t.detach();
        //outputData(r, g, b );
        return true;
    }
    catch(std::exception const& ex)
    {
        m_unreachable = true;
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return false;
}

void TPLinkLight::outputData( uint8_t r ,uint8_t g ,uint8_t b ) {
    setLightOnRGB(r,g,b);
}

void TPLinkLight::RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV) {
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

std::string TPLinkLight::setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp, int period) {
    float h,si,sv,i,v;

    RGBtoHSIV(r/255,g/255,b/255,h,si,sv,i,v);
    
    int ih = (h);
    //int isi = (si*100);
    int isv = (sv*100);
    //int ii = (i*100);
    int iv = (v*100);
    return setLightOnHSV(ih, isv, iv, color_Temp, period);
}

std::string TPLinkLight::setLightOnHSV( int hue, int saturation, int brightness, int color_Temp, int period) {
    //{"smartlife.iot.smartbulb.lightingservice":{"transition_light_state":{"ignore_default":1,"transition_period":150,"mode":"normal","hue":120,"on_off":1,"saturation":65,"color_temp":0,"brightness":10}}}
    
    const std::string cmd = "{\"smartlife.iot.smartbulb.lightingservice\":{\"transition_light_state\":{\"ignore_default\":1,\"transition_period\":" + std::to_string(period) + ",\"mode\":\"normal\",\"hue\":" 
    + std::to_string(hue) + ",\"on_off\":1,\"saturation\":" + std::to_string(saturation) + ",\"color_temp\":" + std::to_string(color_Temp) + ",\"brightness\":" + std::to_string(brightness) + "}}}";
    return sendCmd(cmd);
}

std::string TPLinkLight::setLightOff(){

    const std::string cmd = "{\"smartlife.iot.smartbulb.lightingservice\":{\"transition_light_state\":{\"ignore_default\":1,\"transition_period\":0,\"mode\":\"normal\",\"on_off\":0}}}";
    return sendCmd(cmd);
}