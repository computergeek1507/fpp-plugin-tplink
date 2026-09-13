#pragma once

#include "RelayFollower.h"
#include "RelaySender.h"

#include <cstdint>
#include <utility>

namespace tplink {

// A plug under sequence control: the flip rule in front of the plug's sender.
// BaseSwitch::SendData calls onChannelValue() for every frame.
class SequenceRelay {
public:
    explicit SequenceRelay(RelaySender::SendFn send,
                           RelaySender::Timing timing = RelaySender::defaultTiming(),
                           RelaySender::GiveUpFn onGiveUp = RelaySender::GiveUpFn())
        : m_sender(std::move(send), timing, std::move(onGiveUp)) {}

    void start() { m_sender.start(); }

    // Frame thread. A compare, and only when the ON/OFF state flips, a
    // hand-off to the sender. Never waits on the network and never allocates.
    void onChannelValue(uint8_t value) {
        switch (m_follower.onValue(value)) {
        case RelayFollower::Action::None:
            return;
        case RelayFollower::Action::SwitchOn:
            m_sender.request(true);
            return;
        case RelayFollower::Action::SwitchOff:
            m_sender.request(false);
            return;
        }
    }

    // Resets the follower so the next frame sends, even if the channel is 0.
    // Called when a new playlist starts.
    void reset() { m_follower.reset(); }

    void requestStop() { m_sender.requestStop(); }
    void stop() { m_sender.stop(); }

    RelaySender const& sender() const { return m_sender; }
    RelayFollower const& follower() const { return m_follower; }

private:
    RelayFollower m_follower;
    RelaySender m_sender;
};

}  // namespace tplink
