#include "TPLinkItem.h"

#include "core/KasaProtocol.h"

#include <cerrno>
#include <cstring>
#include <exception>
#include <string>

namespace {

// Port 9999 actively turned us away: the device is there but no longer takes
// the legacy protocol, which is what Kasa firmware that moved to KLAP does.
bool refusedLegacyPort(std::string const& error) {
    return error.find(std::strerror(ECONNREFUSED)) != std::string::npos ||
           error.find(std::strerror(ECONNRESET)) != std::string::npos;
}

}  // namespace

TPLinkItem::TPLinkItem(std::string const& ip, unsigned int startChannel,
                       tplink::tapo::Credentials const& klapCredentials):
    BaseItem(ip,startChannel), m_port(9999), m_klapCredentials(klapCredentials)
{
}

std::shared_ptr<tplink::tapo::Client> TPLinkItem::klapClient() {
    std::lock_guard<std::mutex> lock(m_klapMutex);
    return m_klap;
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
        if (auto klap = klapClient()) {
            tplink::tapo::Options options;
            options.cancel = cancel;
            ok = klap->request(cmd, reply, options, &error);
        } else {
            tplink::kasa::QueryOptions options;
            options.cancel = cancel;
            ok = tplink::kasa::query(m_ipAddress, m_port, cmd, reply, options, &error);
            if (!ok && refusedLegacyPort(error)) {
                // Same commands, carried over KLAP instead. Stay on KLAP only
                // once it has worked, so a plug that is just rebooting is
                // tried on 9999 again next time.
                auto klap = tplink::tapo::Client::shared(m_ipAddress, m_klapCredentials);
                tplink::tapo::Options klapOptions;
                klapOptions.cancel = cancel;
                std::string klapError;
                if (klap->request(cmd, reply, klapOptions, &klapError)) {
                    ok = true;
                    LogInfo(VB_PLUGIN, "TPLink %s: port 9999 is closed; using KLAP for this device\n",
                            m_ipAddress.c_str());
                    std::lock_guard<std::mutex> lock(m_klapMutex);
                    m_klap = klap;
                } else {
                    error += "; KLAP: " + klapError;
                }
            }
        }
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
