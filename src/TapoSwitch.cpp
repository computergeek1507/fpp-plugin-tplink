#include "TapoSwitch.h"

#include "core/TapoClient.h"

#include <string>

TapoSwitch::TapoSwitch(std::string const& ip, unsigned int startChannel, int plug_num,
                       std::string const& username, std::string const& password) :
    BaseItem(ip, startChannel), TapoItem(ip, startChannel, username, password),
    BaseSwitch(ip, startChannel, plug_num)
{
}

TapoSwitch::~TapoSwitch() {
    // The sequence sender calls into this object; stop it first.
    StopSequenceControl();
}

std::string TapoSwitch::GetConfigString() const {
    return "IP: " + GetIPAddress() + " Start Channel: " + std::to_string(GetStartChannel()) +
           " Device Type: " + GetType() + " Plug Number: " + std::to_string(m_plug_num);
}

std::string TapoSwitch::childId(std::atomic<bool> const* cancel) {
    {
        std::lock_guard<std::mutex> lock(m_childIdMutex);
        if (!m_childId.empty()) {
            return m_childId;
        }
    }
    tplink::json::Value result;
    if (!call("get_child_device_list", "{\"start_index\":0}", result, cancel)) {
        return std::string();
    }
    const std::string id = tplink::tapo::childIdAt(result, m_plug_num);
    if (id.empty()) {
        LogWarn(VB_PLUGIN, "Tapo %s has no outlet %d\n", m_ipAddress.c_str(), m_plug_num);
        return id;
    }
    std::lock_guard<std::mutex> lock(m_childIdMutex);
    m_childId = id;
    return id;
}

bool TapoSwitch::setRelay(bool on, std::atomic<bool> const* cancel) {
    const std::string params = on ? "{\"device_on\":true}" : "{\"device_on\":false}";
    tplink::json::Value result;
    if (m_plug_num <= 0) {
        return call("set_device_info", params, result, cancel);
    }
    const std::string id = childId(cancel);
    if (id.empty()) {
        return false;
    }
    return callChild(id, "set_device_info", params, result, cancel);
}

bool TapoSwitch::sendRelayState(bool on, std::atomic<bool> const& stop) {
    return setRelay(on, &stop);
}

bool TapoSwitch::setRelayOn() {
    return setRelay(true, nullptr);
}

bool TapoSwitch::setRelayOff() {
    return setRelay(false, nullptr);
}

// The LED belongs to the whole device, so a strip outlet sets the strip's.
// set_led_info wants the device's full LED settings back with led_rule changed.
bool TapoSwitch::setLed(bool on) {
    tplink::json::Value info;
    if (!call("get_led_info", "", info)) {
        return false;
    }
    std::string params = "{";
    bool first = true;
    if (info.type == tplink::json::Value::Type::Object) {
        for (size_t i = 0; i < info.keys.size() && i < info.values.size(); ++i) {
            if (info.keys[i] == "led_rule") {
                continue;
            }
            params += (first ? "" : ",") + tplink::tapo::quote(info.keys[i]) + ":" + tplink::tapo::toJson(info.values[i]);
            first = false;
        }
    }
    params += std::string(first ? "" : ",") + "\"led_rule\":" + (on ? "\"always\"" : "\"never\"") + "}";
    tplink::json::Value result;
    return call("set_led_info", params, result);
}

bool TapoSwitch::setLedOn() {
    return setLed(true);
}

bool TapoSwitch::setLedOff() {
    return setLed(false);
}
