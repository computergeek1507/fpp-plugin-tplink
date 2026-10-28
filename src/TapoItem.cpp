#include "TapoItem.h"

#include "core/TapoClient.h"

#include <exception>

TapoItem::TapoItem(std::string const& ip, unsigned int startChannel,
                   std::string const& username, std::string const& password) :
    BaseItem(ip, startChannel),
    m_client(tplink::tapo::Client::shared(ip, username, password))
{
    if (username.empty()) {
        LogWarn(VB_PLUGIN, "Tapo %s: no Tapo account set; enter the Tapo email and password in the plugin settings\n",
                ip.c_str());
    }
}

TapoItem::~TapoItem() {
}

std::string TapoItem::getInfo() {
    tplink::json::Value result;
    if (!call("get_device_info", "", result)) {
        return std::string();
    }
    return tplink::tapo::toJson(result);
}

void TapoItem::logResult(char const* method, bool ok, std::string const& error) {
    if (ok) {
        if (m_unreachable.exchange(false)) {
            LogInfo(VB_PLUGIN, "Tapo %s is answering again\n", m_ipAddress.c_str());
        }
        return;
    }
    // Log the first failure loudly, repeats only at debug level.
    if (!m_unreachable.exchange(true)) {
        LogWarn(VB_PLUGIN, "Tapo %s: %s failed: %s\n", m_ipAddress.c_str(), method, error.c_str());
    } else {
        LogDebug(VB_PLUGIN, "Tapo %s: %s failed: %s\n", m_ipAddress.c_str(), method, error.c_str());
    }
}

bool TapoItem::call(std::string const& method, std::string const& paramsJson, tplink::json::Value& result,
                    std::atomic<bool> const* cancel) {
    std::string error;
    bool ok = false;
    try {
        tplink::tapo::Options options;
        options.cancel = cancel;
        ok = m_client->call(method, paramsJson, result, options, &error);
    } catch (std::exception const& ex) {
        error = ex.what();
    }
    logResult(method.c_str(), ok, error);
    return ok;
}

bool TapoItem::callChild(std::string const& childId, std::string const& method, std::string const& paramsJson,
                         tplink::json::Value& result, std::atomic<bool> const* cancel) {
    std::string error;
    bool ok = false;
    try {
        tplink::tapo::Options options;
        options.cancel = cancel;
        ok = m_client->callChild(childId, method, paramsJson, result, options, &error);
    } catch (std::exception const& ex) {
        error = ex.what();
    }
    logResult(method.c_str(), ok, error);
    return ok;
}
