#pragma once


#include "BaseItem.h"

class BaseSwitch : virtual public BaseItem { 
public:
    BaseSwitch(std::string const& ip, unsigned int startChannel  );
    virtual ~BaseSwitch();

    bool SendData( unsigned char *data) override;

    virtual std::string setRelayOn()=0;
    virtual std::string setRelayOff()=0;

    virtual std::string setLedOn()=0;
    virtual std::string setLedOff()=0;

protected:
    uint8_t m_w;

    virtual void outputData( uint8_t w );

};