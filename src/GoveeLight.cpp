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
   //{"msg":{"cmd":"colorwc","data":{"color":{"r":255,"g":0,"b":0},"colorTemInKelvin":0}}
//{"msg":{"cmd":"turn","data":{"value":1}}}
//"{\"msg\":{\"cmd\":\"colorwc\",\"data\":{\"color\":{\"r\":255,\"g\":0,\"b\":0},\"colorTemInKelvin\":0}}}"
   const std::string cmd = "{\"msg\":{\"cmd\":\"colorwc\",\"data\":{\"color\":{\"r\":" + std::to_string(r) + ",\"g\":"
    + std::to_string(g) + ",\"b\":" + std::to_string(b) +"},\"colorTemInKelvin\":" + std::to_string(color_Temp) +"}}}" ;
   return !sendCmd(cmd).empty();
}

std::string GoveeLight::sendCmd(std::string cmd) 
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        std::cerr << "Error creating socket" << std::endl;
         LogInfo(VB_PLUGIN, "Error creating socket\n");
          m_unreachable = true;
        return "";
    }

    // 2. Define the target address
    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(4003); // Govee's default port
    if (inet_pton(AF_INET, m_ipAddress.c_str(), &targetAddr.sin_addr) <= 0) { // Replace with your device's IP
        std::cerr << "Invalid address" << std::endl;
        LogInfo(VB_PLUGIN, "Invalid address\n");
        close(sockfd);
         m_unreachable = true;
        return "";
    }

    // 3. Prepare the message
    //const char* message = cmd.c_str(); // Example command to set color
    //const char* message = "{\"msg\":{\"cmd\":\"colorwc\",\"data\":{\"color\":{\"r\":255,\"g\":0,\"b\":0},\"colorTemInKelvin\":0}}}"; // Example command to set color
    // Example: Set color to red
    //LogInfo(VB_PLUGIN, cmd.c_str());
    // 4. Send the message
    ssize_t bytesSent = sendto(sockfd, cmd.c_str(), cmd.size(), 0, (sockaddr*)&targetAddr, sizeof(targetAddr));
    if (bytesSent == -1) {
        std::cerr << "Error sending message" << std::endl;
        LogInfo(VB_PLUGIN, "Error sending message\n");
    } else {
        std::cout << "Message sent successfully" << std::endl;
        LogInfo(VB_PLUGIN, "Message sent successfully\n");
    }

    // 5. Close the socket
    close(sockfd);
    return "0";
}
