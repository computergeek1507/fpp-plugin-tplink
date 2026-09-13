// Framing, command text and sysinfo ids for the legacy Kasa protocol. The
// socket exchange is in KasaSocket.cpp.

#include "KasaProtocol.h"

#include "MiniJson.h"

namespace tplink {
namespace kasa {

namespace {

constexpr uint8_t kFirstKey = 171;

std::string stringMember(json::Value const* object, char const* key) {
    json::Value const* value = object != nullptr ? object->member(key) : nullptr;
    return value != nullptr ? value->asString() : std::string();
}

}  // namespace

std::string encrypt(std::string const& plain) {
    std::string out(plain.size(), '\0');
    uint8_t key = kFirstKey;
    for (size_t i = 0; i < plain.size(); ++i) {
        const uint8_t c = static_cast<uint8_t>(static_cast<uint8_t>(plain[i]) ^ key);
        out[i] = static_cast<char>(c);
        key = c;
    }
    return out;
}

std::string decrypt(std::string const& cipher) {
    std::string out(cipher.size(), '\0');
    uint8_t key = kFirstKey;
    for (size_t i = 0; i < cipher.size(); ++i) {
        const uint8_t c = static_cast<uint8_t>(cipher[i]);
        out[i] = static_cast<char>(c ^ key);
        key = c;
    }
    return out;
}

std::string frame(std::string const& json) {
    const uint32_t length = static_cast<uint32_t>(json.size());
    std::string out;
    out.reserve(json.size() + 4);
    out.push_back(static_cast<char>((length >> 24) & 0xff));
    out.push_back(static_cast<char>((length >> 16) & 0xff));
    out.push_back(static_cast<char>((length >> 8) & 0xff));
    out.push_back(static_cast<char>(length & 0xff));
    out += encrypt(json);
    return out;
}

std::string sysinfoCommand(int plugNumber) {
    return plugNumber == 0 ? "{\"system\":{\"get_sysinfo\":{}}}"
                           : "{\"system\":{\"get_sysinfo\":{\"children\":{}}}}";
}

std::string relayStateCommand(bool on) {
    return on ? "{\"system\":{\"set_relay_state\":{\"state\":1}}}"
              : "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
}

std::string addressedCommand(std::string const& command, int plugNumber, std::string const& childId) {
    if (plugNumber == 0 || command.empty()) {
        return command;
    }
    return "{\"context\":{\"child_ids\":[\"" + childId + "\"]}," + command.substr(1);
}

std::string idFromSysinfo(std::string const& reply, int plugNumber) {
    json::Value root;
    if (!json::parse(reply, root)) {
        return std::string();
    }
    json::Value const* system = root.member("system");
    json::Value const* info = system != nullptr ? system->member("get_sysinfo") : nullptr;
    if (info == nullptr || plugNumber < 0) {
        return std::string();
    }
    if (plugNumber == 0) {
        return stringMember(info, "deviceId");
    }
    json::Value const* children = info->member("children");
    json::Value const* child =
        children != nullptr ? children->element(static_cast<size_t>(plugNumber - 1)) : nullptr;
    return stringMember(child, "id");
}

}  // namespace kasa
}  // namespace tplink
