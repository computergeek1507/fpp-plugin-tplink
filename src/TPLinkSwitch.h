#pragma once
#include "BaseSwitch.h"
#include "TPLinkItem.h"

#include <atomic>
#include <mutex>
#include <string>

class TPLinkSwitch : public TPLinkItem, public BaseSwitch{
public:
    TPLinkSwitch(std::string const& ip, unsigned int startChannel, int plug_num );
    virtual ~TPLinkSwitch();

    bool setRelayOn() override;
    bool setRelayOff() override;

    bool setLedOn() override;
    bool setLedOff() override;

    std::string GetType() const override { return "TPLinkSwitch"; }
    std::string GetConfigString() const override;

protected:
    bool sendRelayState(bool on, std::atomic<bool> const& stop) override;

private:
    // The sequence sender and the All Switches commands can both read and fill it.
    mutable std::mutex m_deviceIdMutex;
    std::string m_deviceId;

    std::string deviceId() const;
    std::string getDeviceId(int plug_num, std::atomic<bool> const* cancel);
    std::string appendPlugData(std::string const& cmd, std::atomic<bool> const* cancel);
    bool sendRelayCommand(bool on, std::atomic<bool> const* cancel);
};
