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

        // Only act when the ON/OFF state changes. Until the sequence has
        // turned a plug on, an OFF value does nothing: a plug switched on
        // by a command or Home Assistant stays on while the sequence holds
        // its channel at 0.
        if(newState == m_relayState) {
            return true;
        }
        if(m_relayState == 0 && newState == 2) {
            // Unknown state + below half: leave the plug alone.
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

