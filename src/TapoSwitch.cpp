#include "TapoSwitch.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <string>

#include <iostream>

TapoSwitch::TapoSwitch(std::string const& ip, unsigned int startChannel, int plug_num,
                       std::string const& username, std::string const& password) :
    BaseItem(ip, startChannel), TapoItem(ip, startChannel, username, password),
    BaseSwitch(ip, startChannel, plug_num)
{
}

TapoSwitch::~TapoSwitch() {
}

std::string TapoSwitch::GetConfigString() const {
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) +
           " Device Type: " + GetType() + " Plug Number: " + std::to_string(m_plug_num);
}

bool TapoSwitch::setRelayOn() {
    return !sendCmd("on").empty();
}

bool TapoSwitch::setRelayOff() {
    return !sendCmd("off").empty();
}

bool TapoSwitch::setLedOn() {
    // python-kasa does not support LED control on Tapo devices
    return true;
}

bool TapoSwitch::setLedOff() {
    // python-kasa does not support LED control on Tapo devices
    return true;
}
