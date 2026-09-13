#include "BaseSwitch.h"

//#include "common.h"
#include "log.h"

#include <thread>

BaseSwitch::BaseSwitch(std::string const& ip, unsigned int startChannel, int plug_num) :
BaseItem(ip,startChannel),
m_plug_num(plug_num)
{
}

BaseSwitch::~BaseSwitch() {

}

void BaseSwitch::EnableOutput() {
    BaseItem::EnableOutput();
    // A new sequence is starting: send the plug's state on the very next frame.
    m_relayState = 0;
}

bool BaseSwitch::SendData( unsigned char *data) {
    try
    {
        if(m_unreachable){
            return false;
        }

        if(m_startChannel == 0){
            return false;
        }

        uint8_t w = data[m_startChannel - 1];
        uint8_t newState = (w >= 127) ? 1 : 2;  // 1 = on, 2 = off

        // A start channel means the sequence owns the plug. On the first
        // frame after a sequence starts (m_relayState == 0) we always send,
        // even if the channel is 0. After that we send only when the ON/OFF
        // state changes. No periodic re-send.
        if(newState == m_relayState) {
            return true;
        }
        m_relayState = newState;

        std::thread t(&BaseSwitch::outputData, this, w );
        t.detach();
        return true;
    }
    catch(std::exception const& ex)
    {
        m_unreachable = true;
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return false;
}

void BaseSwitch::outputData( uint8_t w ) {
    if(w >= 127){
        setRelayOn();
    } else {
        setRelayOff();
    }
}

