#pragma once

#include "BaseSwitch.h"
#include "KasaKLAPItem.h"

#include <atomic>
#include <mutex>
#include <string>

// A KLAP plug, or (plug number 1 and up) one outlet of a KLAP power strip.
class KasaKLAPSwitch : public KasaKLAPItem, public BaseSwitch {
public:
    KasaKLAPSwitch(std::string const& ip, unsigned int startChannel, int plug_num,
               tplink::tapo::Credentials const& credentials);
    virtual ~KasaKLAPSwitch();

    bool setRelayOn() override;
    bool setRelayOff() override;

    bool setLedOn() override;
    bool setLedOff() override;

    std::string GetType() const override { return "KasaKLAPSwitch"; }
    std::string GetConfigString() const override;

protected:
    bool sendRelayState(bool on, std::atomic<bool> const& stop) override;

private:
    bool setRelay(bool on, std::atomic<bool> const* cancel);
    bool setLed(bool on);
    // The strip outlet's device_id, looked up once and cached.
    std::string childId(std::atomic<bool> const* cancel);

    std::mutex m_childIdMutex;
    std::string m_childId;
};
