#include "TasmotaLight.h"

//#include <stdlib.h>
//#include <cstdint>
#include <thread>
//#include <cmath>

//#include <arpa/inet.h>  // for inet_pton
//#include <netinet/in.h> // for sockaddr_in, htons
//#include <stdint.h>     // for uint16_t, uint8_t, uint32_t
//#include <stdio.h>      // for printf
//#include <sys/socket.h> // for AF_INET, connect, send, socket, SOCK_STREAM
//#include <unistd.h>     // for close, read
#include <string>       // for string
//#include <cstring>      // for ??? memcpy, memset, strncpy


#include <iostream>
#include <istream>
#include <ostream>

TasmotaLight::TasmotaLight(std::string const& ip, unsigned int startChannel):
 BaseItem(ip,startChannel), BaseLight(ip,startChannel), m_curl(NULL)
{
    m_curl = curl_easy_init();
}

TasmotaLight::~TasmotaLight() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

bool TasmotaLight::setLightOnHSV( int hue, int saturation, int brightness, int color_Temp, int period) {
    std::string repURL = "http://" + m_ipAddress + "/cm?cmnd=HSBColor%20" + std::to_string(hue)
    + "," + std::to_string(saturation) + ","  + std::to_string(brightness);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_URL, repURL.c_str());

    CURLcode status = curl_easy_perform(m_curl);
    if (status != CURLE_OK) {
        m_unreachable = true;
        std::cout << "failed to send color command\n";
        return false;
    }
    //HSBColor
    return true;
}

bool TasmotaLight::setLightOff(){

    std::string repURL = "http://" + m_ipAddress + "/cm?cmnd=Power%20Off";
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_URL, repURL.c_str());

    CURLcode status = curl_easy_perform(m_curl);
    if (status != CURLE_OK) {
        m_unreachable = true;
        std::cout << "failed to send on command\n";
        return false;
    }
    m_unreachable = false;
    return true;
}


bool TasmotaLight::setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp, int period) {
    std::string repURL = "http://" + m_ipAddress + "/cm?cmnd=Color%20" + std::to_string(r)
    + "," + std::to_string(g) + ","  + std::to_string(b);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_URL, repURL.c_str());

    CURLcode status = curl_easy_perform(m_curl);
    if (status != CURLE_OK) {
        m_unreachable = true;
        std::cout << "failed to send color command\n";
        return false;
    }
    return true;
}