#pragma once

#include "BaseSwitch.h"
#include "TapoItem.h"

#include <string>

class TapoSwitch : public TapoItem, public BaseSwitch {
public:
    TapoSwitch(std::string const& ip, unsigned int startChannel, int plug_num,
               std::string const& username = "", std::string const& password = "");
    virtual ~TapoSwitch();

    bool setRelayOn() override;
    bool setRelayOff() override;

    bool setLedOn() override;
    bool setLedOff() override;

    std::string GetType() const override { return "TapoSwitch"; }
    std::string GetConfigString() const override;
};
