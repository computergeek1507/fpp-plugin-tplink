#pragma once
#include "BaseSwitch.h"
#include "TPLinkItem.h"

#include <string>

class TPLinkSwitch : public TPLinkItem, public BaseSwitch{
public:
    TPLinkSwitch(std::string const& ip, unsigned int startChannel, int plug_num );
    virtual ~TPLinkSwitch();

    std::string setRelayOn() override;
    std::string setRelayOff()override;

    std::string setLedOn() override;
    std::string setLedOff()override;

    std::string GetType() const override {return "Switch";}
    std::string GetConfigString() const override;

private:
    int m_plug_num;
    std::string m_deviceId;

    std::string getDeviceId(int plug_num);
    std::string appendPlugData(std::string cmd);
};