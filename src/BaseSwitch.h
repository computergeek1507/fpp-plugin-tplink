#pragma once


#include "BaseItem.h"

class BaseSwitch : virtual public BaseItem { 
public:
    BaseSwitch(std::string const& ip, unsigned int startChannel , int plug_num );
    virtual ~BaseSwitch();

    bool SendData( unsigned char *data) override;

    virtual bool setRelayOn() = 0;
    virtual bool setRelayOff() = 0;

    virtual bool setLedOn() = 0;
    virtual bool setLedOff() = 0;

protected:
    uint8_t m_w;
    int m_plug_num;

    virtual void outputData( uint8_t w );

};