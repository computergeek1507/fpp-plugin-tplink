#include "BaseSwitch.h"

//#include "common.h"
#include "log.h"

#include <exception>
#include <string>

namespace {

// "2 min", "45 s" or "1500 ms", for the log.
std::string describeWindow(std::chrono::milliseconds window) {
    const long long ms = window.count();
    if (ms > 0 && ms % 60000 == 0) {
        return std::to_string(ms / 60000) + " min";
    }
    if (ms % 1000 == 0) {
        return std::to_string(ms / 1000) + " s";
    }
    return std::to_string(ms) + " ms";
}

}  // namespace

BaseSwitch::BaseSwitch(std::string const& ip, unsigned int startChannel, int plug_num) :
BaseItem(ip,startChannel),
m_plug_num(plug_num)
{
}

BaseSwitch::~BaseSwitch() {
    StopSequenceControl();
}

void BaseSwitch::EnableOutput() {
    BaseItem::EnableOutput();
    // A new sequence is starting: reset the follower so the very next frame
    // sends the channel's state, even if it is 0.
    if (m_sequence) {
        m_sequence->reset();
    }
}

// A plug with a start channel follows that channel: 127 or more is on, below
// 127 is off. The sequence owns the plug: when a playlist starts, the first
// frame sends the state; after that, only changes are sent.
bool BaseSwitch::SendData( unsigned char *data) {
    if (m_startChannel == 0 || !m_sequence) {
        return false;
    }
    try {
        m_sequence->onChannelValue(data[m_startChannel - 1]);
    } catch (std::exception const& ex) {
        LogInfo(VB_PLUGIN, "Error %s \n", ex.what());
        return false;
    }
    return true;
}

void BaseSwitch::StartSequenceControl() {
    if (m_startChannel == 0 || m_sequence) {
        return;
    }
    try {
        auto sequence = std::make_unique<tplink::SequenceRelay>(
            [this](bool on, std::atomic<bool> const& stop) { return sendForSequence(on, stop); },
            tplink::RelaySender::defaultTiming(),
            [this](bool on, unsigned tries, std::chrono::milliseconds window) {
                gaveUpForSequence(on, tries, window);
            });
        sequence->start();
        m_sequence = std::move(sequence);
    } catch (std::exception const& ex) {
        LogInfo(VB_PLUGIN, "Could not start sequence control for %s: %s\n", m_ipAddress.c_str(), ex.what());
    }
}

void BaseSwitch::RequestStopSequenceControl() {
    if (m_sequence) {
        m_sequence->requestStop();
    }
}

void BaseSwitch::StopSequenceControl() {
    if (m_sequence) {
        m_sequence->stop();
    }
}

bool BaseSwitch::sendRelayState(bool on, std::atomic<bool> const&) {
    return on ? setRelayOn() : setRelayOff();
}

bool BaseSwitch::sendForSequence(bool on, std::atomic<bool> const& stop) {
    const bool ok = sendRelayState(on, stop);
    if (ok) {
        if (m_failedTries > 0) {
            LogInfo(VB_PLUGIN, "Switched %s %s after %u failed tries\n", m_ipAddress.c_str(), on ? "on" : "off",
                    m_failedTries);
        }
        m_failedTries = 0;
    } else if (!stop.load()) {
        if (m_failedTries == 0) {
            LogInfo(VB_PLUGIN, "Could not switch %s %s; retrying for up to %s\n", m_ipAddress.c_str(),
                    on ? "on" : "off", describeWindow(tplink::RelaySender::defaultTiming().giveUpAfter).c_str());
        }
        ++m_failedTries;
    }
    return ok;
}

void BaseSwitch::gaveUpForSequence(bool on, unsigned tries, std::chrono::milliseconds window) {
    LogInfo(VB_PLUGIN, "Gave up switching %s %s after %u tries over %s; the next change on its channel will try again\n",
            m_ipAddress.c_str(), on ? "on" : "off", tries, describeWindow(window).c_str());
    m_failedTries = 0;
}
