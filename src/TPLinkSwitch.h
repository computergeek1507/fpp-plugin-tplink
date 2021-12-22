#pragma once

#include "TPLinkItem.h"

#include <string>

class TPLinkSwitch : public TPLinkItem{
public:
    TPLinkSwitch(std::string const& ip, unsigned int startChannel, int plug_num );
    virtual ~TPLinkSwitch();

    bool SendData( unsigned char *data) override;

    std::string setRelayOn();
    std::string setRelayOff();

    std::string setLedOn();
    std::string setLedOff();

    std::string GetType() const override {return "Switch";}
    std::string GetConfigString() const override;

private:
    uint8_t m_w;
    int m_plug_num;
    std::string m_deviceId;

    void outputData( uint8_t w );
    std::string getDeviceId(int plug_num);
    std::string appendPlugData(std::string cmd);
};