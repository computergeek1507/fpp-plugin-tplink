#include "BaseSwitch.h"

//#include "common.h"
#include "log.h"

#include <thread>

BaseSwitch::BaseSwitch(std::string const& ip, unsigned int startChannel) :
BaseItem(ip,startChannel),
m_w(0)
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

        if(w == m_w ) {
            if(m_seqCount < 1201) {
                ++ m_seqCount;
                return true;
            }
        }
        m_seqCount=0;
        m_w = w;

        std::thread t(&BaseSwitch::outputData, this, w );
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

void BaseSwitch::outputData( uint8_t w ) {
    if(w >= 127){
        setRelayOn();
    } else {
        setRelayOff();
    }
}

