#include "TapoSwitch.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <iostream>
#include <istream>
#include <ostream>

TapoSwitch::TapoSwitch(std::string const& ip, unsigned int startChannel, int plug_num) :
BaseItem(ip,startChannel), TapoItem(ip,startChannel), BaseSwitch(ip,startChannel,plug_num)
{
}

TapoSwitch::~TapoSwitch() {

}

std::string TapoSwitch::GetConfigString() const
{
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) + " Device Type: " + GetType() + 
    " Plug Number: " + std::to_string(m_plug_num);
}

bool TapoSwitch::setRelayOn() {
    const std::string cmd = "on";
    sendCmd(cmd);
    return true;
}

bool TapoSwitch::setRelayOff() {
    const std::string cmd = "off";
    sendCmd(cmd);
    return true;
}

bool TapoSwitch::setLedOff() {
    const std::string cmd = "led off";
    sendCmd(cmd);
    return true;
}

bool TapoSwitch::setLedOn() {
    const std::string cmd = "led on";
    sendCmd(cmd);
    return true;
}
