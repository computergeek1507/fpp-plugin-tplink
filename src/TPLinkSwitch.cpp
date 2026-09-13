#include "TPLinkSwitch.h"

#include "core/KasaProtocol.h"

#include "fpp-pch.h"
#include "common.h"
#include "settings.h"

#include <exception>
#include <string>

TPLinkSwitch::TPLinkSwitch(std::string const& ip, unsigned int startChannel, int plug_num) :
BaseItem(ip,startChannel), TPLinkItem(ip,startChannel), BaseSwitch(ip,startChannel,plug_num)
{
    m_deviceId = getDeviceId(plug_num, nullptr);
}

TPLinkSwitch::~TPLinkSwitch() {
    // The sequence sender calls into this object; stop it before anything here goes away.
    StopSequenceControl();
}

std::string TPLinkSwitch::GetConfigString() const
{
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) + " Device Type: " + GetType() +
    " Plug Number: " + std::to_string(m_plug_num) + " Device ID: " + deviceId();
}

std::string TPLinkSwitch::deviceId() const {
    std::lock_guard<std::mutex> lock(m_deviceIdMutex);
    return m_deviceId;
}

std::string TPLinkSwitch::getDeviceId(int plug_num, std::atomic<bool> const* cancel) {
    try {
        const std::string reply = sendCmd(tplink::kasa::sysinfoCommand(plug_num), cancel);
        if (reply.empty()) {
            LogInfo(VB_PLUGIN, "No sysinfo returned\n");
            return "";
        }
        const std::string id = tplink::kasa::idFromSysinfo(reply, plug_num);
        if (id.empty()) {
            LogInfo(VB_PLUGIN, "No device id for plug %d in sysinfo %s\n", plug_num, reply.c_str());
        }
        return id;
    }
    catch(std::exception const& ex) {
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return "";
}

bool TPLinkSwitch::setRelayOn() {
    return sendRelayCommand(true, nullptr);
}

bool TPLinkSwitch::setRelayOff() {
    return sendRelayCommand(false, nullptr);
}

bool TPLinkSwitch::sendRelayState(bool on, std::atomic<bool> const& stop) {
    return sendRelayCommand(on, &stop);
}

bool TPLinkSwitch::sendRelayCommand(bool on, std::atomic<bool> const* cancel) {
    return !sendCmd(appendPlugData(tplink::kasa::relayStateCommand(on), cancel), cancel).empty();
}

bool TPLinkSwitch::setLedOff() {
    const std::string cmd = "{\"system\":{\"set_led_off\":{\"off\":1}}}";
    return !sendCmd(appendPlugData(cmd, nullptr)).empty();
}

bool TPLinkSwitch::setLedOn() {
    const std::string cmd = "{\"system\":{\"set_led_off\":{\"off\":0}}}";
    return !sendCmd(appendPlugData(cmd, nullptr)).empty();
}

std::string TPLinkSwitch::appendPlugData(std::string const& cmd, std::atomic<bool> const* cancel) {
    if (m_plug_num == 0) {
        return cmd;
    }
    std::string id = deviceId();
    if (id.empty()) {
        id = getDeviceId(m_plug_num, cancel);
        if (!id.empty()) {
            std::lock_guard<std::mutex> lock(m_deviceIdMutex);
            m_deviceId = id;
        }
    }
    if (id.empty()) {
        LogInfo(VB_PLUGIN, "DeviceId is empty for %s \n", m_ipAddress.c_str());
    }
    return tplink::kasa::addressedCommand(cmd, m_plug_num, id);
}
