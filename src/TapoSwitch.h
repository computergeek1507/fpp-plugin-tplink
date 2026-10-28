#pragma once

#include "BaseSwitch.h"
#include "TapoItem.h"

#include <atomic>
#include <mutex>
#include <string>

// A Tapo plug, or (plug number 1 and up) one outlet of a Tapo power strip.
class TapoSwitch : public TapoItem, public BaseSwitch {
public:
    TapoSwitch(std::string const& ip, unsigned int startChannel, int plug_num,
               tplink::tapo::Credentials const& credentials);
    virtual ~TapoSwitch();

    bool setRelayOn() override;
    bool setRelayOff() override;

    bool setLedOn() override;
    bool setLedOff() override;

    std::string GetType() const override { return "TapoSwitch"; }
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
