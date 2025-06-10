#include "BaseItem.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <stdint.h>     // for uint16_t, uint8_t, uint32_t
#include <stdio.h>      // for printf

#include <unistd.h>     // for close, read
#include <string>       // for string


#include <iostream>
#include <istream>
#include <ostream>

BaseItem::BaseItem(std::string const& ip, unsigned int startChannel) :
    m_ipAddress(ip),
    //m_port(9999),
    m_startChannel(startChannel),
    m_seqCount(0),
    m_unreachable(false),
    m_issending(false)
{
}

BaseItem::~BaseItem() {

}

std::string BaseItem::GetConfigString() const {
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) + " Device Type: " + GetType();
}



