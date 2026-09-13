#pragma once

#include "BaseItem.h"

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
#include "log.h"

class TPLinkItem  : virtual public BaseItem {
public:
    TPLinkItem(std::string const& ip, unsigned int startChannel );
    virtual ~TPLinkItem();

    std::string getInfo();

protected:
    // Sends one command and returns the plug's decrypted reply, or "" when it
    // fails. When `cancel` is given, a slow exchange gives up as soon as it
    // turns true.
    std::string sendCmd(std::string const& cmd, std::atomic<bool> const* cancel = nullptr);

private:
    uint16_t m_port{9999};
};
