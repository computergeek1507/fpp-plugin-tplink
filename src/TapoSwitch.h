#pragma once

#include "BaseSwitch.h"
#include "TapoItem.h"

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
#include "log.h"

class TapoSwitch : public TapoItem, public BaseSwitch {
public:
    TapoSwitch(std::string const& ip, unsigned int startChannel, int plug_num );
    virtual ~TapoSwitch();

    bool setRelayOn() override;
    bool setRelayOff() override;

    bool setLedOn() override;
    bool setLedOff() override;

    std::string GetType() const override { return "TapoSwitch"; }
    std::string GetConfigString() const override;


protected:


private:

};