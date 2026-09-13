#include "TPLinkItem.h"

#include "core/KasaProtocol.h"

#include <exception>
#include <string>

TPLinkItem::TPLinkItem(std::string const& ip, unsigned int startChannel ):
    BaseItem(ip,startChannel), m_port(9999)
{
}

TPLinkItem::~TPLinkItem() {

}

std::string TPLinkItem::getInfo() {
    return sendCmd(tplink::kasa::sysinfoCommand(0));
}

std::string TPLinkItem::sendCmd(std::string const& cmd, std::atomic<bool> const* cancel) {
    // One exchange per item at a time, as before: a send that finds another in
    // flight on this item is dropped and reports failure.
    if (m_issending.exchange(true)) {
        return std::string();
    }
    std::string reply;
    std::string error;
    bool ok = false;
    try {
        tplink::kasa::QueryOptions options;
        options.cancel = cancel;
        ok = tplink::kasa::query(m_ipAddress, m_port, cmd, reply, options, &error);
    } catch (std::exception const& ex) {
        error = ex.what();
    }
    m_issending = false;
    if (!ok) {
        LogDebug(VB_PLUGIN, "TPLink %s: %s\n", m_ipAddress.c_str(), error.c_str());
        return std::string();
    }
    return reply;
}
