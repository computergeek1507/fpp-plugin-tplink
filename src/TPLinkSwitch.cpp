#include "TPLinkSwitch.h"

#include "fpp-pch.h"
#include "common.h"
#include "settings.h"

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

TPLinkSwitch::TPLinkSwitch(std::string const& ip, unsigned int startChannel, int plug_num) :
    TPLinkItem(ip,startChannel),
    m_w(0),
    m_plug_num(plug_num)
{
    m_deviceId = getDeviceId(plug_num);
}

TPLinkSwitch::~TPLinkSwitch() {

}

std::string TPLinkSwitch::GetConfigString() const
{
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) + " Device Type: " + GetType() + 
    " Plug Number: " + std::to_string(m_plug_num) + " Device ID: " + m_deviceId;
}

bool TPLinkSwitch::SendData( unsigned char *data) {
    try
    {
        if(m_unreachable){
            return false;
        }

        uint8_t w = data[m_startChannel - 1];

        if(w == m_w ) {
            if(m_seqCount < 1201) {
                ++ m_seqCount;
                return true;
            }
        }
        m_seqCount=0;
        m_w = w;

        std::thread t(&TPLinkSwitch::outputData, this, w );
        t.detach();
        //outputData(w );
        return true;
    }
    catch(std::exception const& ex)
    {
        m_unreachable = true;
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return false;
}

void TPLinkSwitch::outputData( uint8_t w ) {
    if(w >= 127){
        setRelayOn();
    } else {
        setRelayOff();
    }
}


std::string TPLinkSwitch::getDeviceId(int plug_num) {

    try {
        if(plug_num == 0) {
            const std::string cmd2 = "{\"system\":{\"get_sysinfo\":{}}}";
            auto data2 = sendCmd(cmd2);
            if(data2.empty()){
                LogInfo(VB_PLUGIN, "No sysinfo returned\n");
                return "";
            }
            Json::Value jsonData2;
            bool result2 = LoadJsonFromString(data2, jsonData2);
            if(!result2 || jsonData2.size() == 0) {
                LogInfo(VB_PLUGIN, "Invalid JSON returned %s\n", data2.c_str());
                return "";
            }
            return jsonData2["system"]["get_sysinfo"]["deviceId"].asString();
        }
        const std::string cmd = "{\"system\":{\"get_sysinfo\":{\"children\":{}}}}";
        std::string data = sendCmd(cmd);
        if(data.empty()){
            LogInfo(VB_PLUGIN, "No sysinfo returned\n");
            return "";
        }
        Json::Value jsonData;
        bool result = LoadJsonFromString(data, jsonData);
        if(result && jsonData.size() != 0) {
            return jsonData["system"]["get_sysinfo"]["children"][plug_num - 1]["id"].asString();
        }
    }
    catch(std::exception const& ex) {
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return "";
}

std::string TPLinkSwitch::setRelayOn() {
    const std::string cmd = "{\"system\":{\"set_relay_state\":{\"state\":1}}}";
    return sendCmd(appendPlugData(cmd));
}

std::string TPLinkSwitch::setRelayOff() {
    const std::string cmd = "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
    return sendCmd(appendPlugData(cmd));
}

std::string TPLinkSwitch::setLedOff() {
    const std::string cmd = "{\"system\":{\"set_led_off\":{\"off\":1}}}";
    return sendCmd(appendPlugData(cmd));
}

std::string TPLinkSwitch::setLedOn() {
    const std::string cmd = "{\"system\":{\"set_led_off\":{\"off\":0}}}";
    return sendCmd(appendPlugData(cmd));
}


std::string TPLinkSwitch::appendPlugData(std::string cmd) {

    if(m_plug_num != 0) {
        if(m_deviceId.empty()) {
            m_deviceId = getDeviceId(m_plug_num);
        }
        if(m_deviceId.empty()) {
            LogInfo(VB_PLUGIN, "DeviceId is empty for %s \n", m_ipAddress.c_str());
        }

        cmd.erase(0, 1);//remove first parentheses
        cmd = "{\"context\":{\"child_ids\":[\"" + m_deviceId + "\"]}," + cmd;
    }
    return cmd;
}


