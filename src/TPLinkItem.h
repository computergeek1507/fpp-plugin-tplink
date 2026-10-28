#pragma once

#include "BaseItem.h"

#include "core/TapoClient.h"

#include <memory>
#include <mutex>

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
#include "log.h"

class TPLinkItem  : virtual public BaseItem {
public:
    // `klapCredentials` are used only if the device turns out to have moved its
    // commands from TCP 9999 onto KLAP, as some Kasa firmware updates do.
    TPLinkItem(std::string const& ip, unsigned int startChannel,
               tplink::tapo::Credentials const& klapCredentials = tplink::tapo::Credentials());
    virtual ~TPLinkItem();

    std::string getInfo();

protected:
    // Sends one command and returns the plug's decrypted reply, or "" when it
    // fails. When `cancel` is given, a slow exchange gives up as soon as it
    // turns true.
    std::string sendCmd(std::string const& cmd, std::atomic<bool> const* cancel = nullptr);

private:
    // The KLAP client once the device has answered over KLAP, else null.
    std::shared_ptr<tplink::tapo::Client> klapClient();

    uint16_t m_port{9999};
    tplink::tapo::Credentials m_klapCredentials;
    std::mutex m_klapMutex;
    std::shared_ptr<tplink::tapo::Client> m_klap;
};
