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
    int m_plug_num;

    virtual void outputData( uint8_t w );

private:
    // The ON/OFF state the sequence last drove the plug to.
    // 0 = unknown (no sequence has turned it on yet), 1 = on, 2 = off.
    uint8_t m_relayState{0};
};
