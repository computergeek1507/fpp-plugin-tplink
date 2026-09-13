#pragma once

#include <cstdint>

namespace tplink {

// Decides when a plug's sequence channel switches the plug.
//
// A start channel means the sequence owns the plug. A channel value of 127 or
// more means ON; below 127 means OFF. On the first value after construction or
// reset(), the state is always sent (even OFF). After that the plug is switched
// only when the ON/OFF state changes.
//
// reset() is called when a new playlist starts, so the first frame of each
// sequence sends the plug's state immediately.
class RelayFollower {
public:
    enum class Action : uint8_t { None, SwitchOn, SwitchOff };

    static constexpr uint8_t kOnThreshold = 127;

    Action onValue(uint8_t value) noexcept {
        const State now = value >= kOnThreshold ? State::On : State::Off;
        if (now == m_state) {
            return Action::None;
        }
        m_state = now;
        return now == State::On ? Action::SwitchOn : Action::SwitchOff;
    }

    // Resets so the next onValue() always sends, even if the channel is 0.
    void reset() noexcept { m_state = State::NeedsSend; }

    // True once at least one value has been sent.
    bool hasSent() const noexcept { return m_state != State::NeedsSend; }

private:
    enum class State : uint8_t { NeedsSend, On, Off };
    State m_state = State::NeedsSend;
};

}  // namespace tplink
