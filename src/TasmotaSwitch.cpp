#include "TasmotaSwitch.h"

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
//#include <string>       // for string
//#include <cstring>      // for ??? memcpy, memset, strncpy


#include <iostream>
#include <istream>
#include <ostream>

TasmotaSwitch::TasmotaSwitch(std::string const& ip, unsigned int startChannel, int plug_num):
 BaseItem(ip,startChannel), BaseSwitch(ip,startChannel,plug_num), m_curl(NULL)
{
    m_curl = curl_easy_init();
}

TasmotaSwitch::~TasmotaSwitch() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

std::string TasmotaSwitch::GetConfigString() const {
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) + " Device Type: " + GetType();
}

bool TasmotaSwitch::setRelayOn() {
    std::string repURL = "http://" + m_ipAddress + "/cm?cmnd=Power%20On";
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

bool TasmotaSwitch::setRelayOff() {
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

bool TasmotaSwitch::setLedOff() {
    std::string repURL = "http://" + m_ipAddress + "/cm?cmnd=LedPower%20Off";
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

bool TasmotaSwitch::setLedOn() {
    std::string repURL = "http://" + m_ipAddress + "/cm?cmnd=LedPower%20On";
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


