#pragma once

#include "BaseItem.h"

#include "core/MiniJson.h"

#include <atomic>
#include <memory>
#include <string>

#include "log.h"

namespace tplink {
namespace tapo {
class Client;
}
}

// A Tapo device, spoken to natively over KLAP (see core/TapoClient.h). All
// Tapo devices share the one Tapo account set in the plugin's settings.
class TapoItem : virtual public BaseItem {
public:
    TapoItem(std::string const& ip, unsigned int startChannel,
             std::string const& username, std::string const& password);
    virtual ~TapoItem();

    // get_device_info's result as JSON text, or "" when it fails.
    std::string getInfo();

protected:
    // One SMART request; logs and returns false on failure. When `cancel` is
    // given, a slow exchange gives up as soon as it turns true.
    bool call(std::string const& method, std::string const& paramsJson, tplink::json::Value& result,
              std::atomic<bool> const* cancel = nullptr);
    bool callChild(std::string const& childId, std::string const& method, std::string const& paramsJson,
                   tplink::json::Value& result, std::atomic<bool> const* cancel = nullptr);

    std::shared_ptr<tplink::tapo::Client> m_client;

private:
    void logResult(char const* method, bool ok, std::string const& error);
};
