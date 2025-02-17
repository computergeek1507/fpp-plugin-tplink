#include "GoveeLight.h"

#include "log.h"

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

#include <netinet/tcp.h>

#include <iostream>
#include <istream>
#include <ostream>

GoveeLight::GoveeLight(std::string const& ip, unsigned int startChannel):
 BaseItem(ip,startChannel), BaseLight(ip,startChannel)
 {

 }

GoveeLight::~GoveeLight() {
    
}

std::string GoveeLight::GetConfigString() const {
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) + " Device Type: " + GetType();
}

bool GoveeLight::setLightOnHSV( int hue, int saturation, int brightness, int color_Temp, int period) {
    //{"smartlife.iot.smartbulb.lightingservice":{"transition_light_state":{"ignore_default":1,"transition_period":150,"mode":"normal","hue":120,"on_off":1,"saturation":65,"color_temp":0,"brightness":10}}}
    
    const std::string cmd = "{\"smartlife.iot.smartbulb.lightingservice\":{\"transition_light_state\":{\"ignore_default\":1,\"transition_period\":" + std::to_string(period) + ",\"mode\":\"normal\",\"hue\":" 
    + std::to_string(hue) + ",\"on_off\":1,\"saturation\":" + std::to_string(saturation) + ",\"color_temp\":" + std::to_string(color_Temp) + ",\"brightness\":" + std::to_string(brightness) + "}}}";
    return !sendCmd(cmd).empty();
}

bool GoveeLight::setLightOff(){

    const std::string cmd = "{\"msg\":{\"cmd\":\"turn\",\"data\":{\"value\":\"0\"}}}" ;
    return !sendCmd(cmd).empty();
}


bool GoveeLight::setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp, int period) {
    /*
    "msg":{
                        "cmd":"colorwc",
                        "data":{
                            "color":{"r":color[0],"g":color[1],"b":color[2]},
                            "colorTemInKelvin":0
                        }    
                    }
    */

   const std::string cmd = "{\"msg\":{\"cmd\":\"colorwc\",\"data\":{\"color\":{\"r\":" + std::to_string(r) + ",\"g\":"
    + std::to_string(g) + ",\"b\":" + std::to_string(b) +"},\"colorTemInKelvin\":" + std::to_string(color_Temp) +"}}" ;
   return !sendCmd(cmd).empty();
}

std::string GoveeLight::sendCmd(std::string const& cmd) {
    try {
        char encrypted[cmd.length()];
        std::memcpy(encrypted, const_cast<char *>(cmd.c_str()), cmd.length());
        //encryptWithHeader(encrypted, const_cast<char *>(cmd.c_str()), cmd.length());
        char response[2048] = {0};

        uint16_t length = sockConnect(response, m_ipAddress.c_str(), m_port, encrypted, cmd.length() + 4);
        if (length == 0) {
            return std::string("");
        }
        return std::string(response);
    }
    catch(std::exception const& ex) {
        LogInfo(VB_PLUGIN, "Error %s \n", ex.what());
    }
    return std::string("");
}

uint16_t GoveeLight::sockConnect(char *out, const char *ip_add, int port, const char *cmd, uint16_t length) {
    if(m_issending)
    {
        return 0;
    }
    m_issending = true;
    //struct sockaddr_in address;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buf[2048] = {0};
    //  char buffer[2048] = {0};
    //    char buffer[2048] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        m_issending = false;
        return 0;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip_add, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        m_issending = false;
        return 0;
    }

    int synRetries = 2; // Send a total of 3 SYN packets => Timeout ~7s
    setsockopt(sock, IPPROTO_TCP, TCP_SYNCNT, &synRetries, sizeof(synRetries));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        m_issending = false;
        return 0;
    }
    send(sock, cmd, length, 0);

    int br = recv(sock, buf, 2048, 0);
    int dataLen = 0;
    int valread = 0;
    if (br > 4) {
        dataLen = ((int)buf[2] << 8) + (int)buf[3];
        valread = br;
        while (br >= 0 && (valread < (dataLen + 4))) {
            br = recv(sock, buf + valread, 2048 - valread, 0);
            if (br > 0) {
                valread += br;
            }
        }
    }
    close(sock);

    if (valread == 0) {
        printf("\nNo bytes read\n");
    } else {
        // buf + 4 strips 4 byte header
        // valread - 3 leaves 1 byte for terminating null character
        strncpy(out, buf + 4, valread - 3);
    }
    m_issending = false;
    return valread;
}