#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

namespace tplink {

// The one sender a plug has for sequence control.
//
// It owns a single thread that does all of this plug's sequence I/O, so sends
// never overlap and never land out of order. The frame thread hands it a
// desired state with request(): lock-free stores plus a wake-up, with no
// allocation and no network I/O. The sender always sends the newest desired
// state, so states superseded while a send was in flight are skipped.
//
// A failed send is retried after a delay that starts at Timing::firstRetry and
// doubles up to Timing::maxRetry. Retrying stops once Timing::giveUpAfter has
// passed since that state was requested: the state is abandoned, never counted
// as delivered, and the sender waits for a newer request. So a plug that was
// offline when its channel changed does not switch itself hours later when it
// comes back. A newer request ends any wait at once, is sent straight away, and
// starts again from the first retry delay with a window of its own.
//
// Every atomic here is 32-bit, so the hand-off stays lock-free on all the ARM
// boards FPP runs on.
class RelaySender {
public:
    // Switches the relay; runs only on the sender thread. Returns true when the
    // plug acknowledged. `stop` turns true when the sender is told to stop, and
    // slow I/O should give up when it does.
    using SendFn = std::function<bool(bool on, std::atomic<bool> const& stop)>;

    // Called on the sender thread, once for each abandoned state, with how many
    // tries it had and the window they were made in.
    using GiveUpFn = std::function<void(bool on, unsigned tries, std::chrono::milliseconds window)>;

    struct Timing {
        std::chrono::milliseconds firstRetry{250};
        std::chrono::milliseconds maxRetry{8000};
        std::chrono::milliseconds giveUpAfter{std::chrono::minutes(2)};
    };

    struct Stats {
        uint64_t attempts;
        uint64_t delivered;
        uint64_t failed;
        uint64_t abandoned;
    };

    // Tries after 250 ms, 500 ms, 1 s, 2 s, 4 s, then every 8 s, for up to 2 minutes.
    static Timing defaultTiming();

    RelaySender(SendFn send, Timing timing, GiveUpFn onGiveUp = GiveUpFn());
    ~RelaySender();

    RelaySender(RelaySender const&) = delete;
    RelaySender& operator=(RelaySender const&) = delete;

    // Starts the sender thread. Throws std::system_error if it cannot.
    void start();

    // Frame thread: asks for the relay to be on or off.
    void request(bool on);

    // Tells the sender to stop without waiting for it. An exchange in flight
    // sees `stop` turn true.
    void requestStop();

    // Stops the sender and waits for its thread. Safe to call more than once.
    void stop();

    // True when nothing is left to send: the newest request was delivered or
    // abandoned, or nothing has been requested.
    bool idle() const;

    // True when the newest request was abandoned.
    bool gaveUp() const;

    Stats stats() const;

private:
    bool pending() const;
    void run();
    bool attempt(bool on);
    void abandon(std::unique_lock<std::mutex>& lock, uint32_t word, unsigned tries);
    void wake();

    SendFn m_send;
    Timing m_timing;
    GiveUpFn m_onGiveUp;

    // The newest desired state as (generation << 1) | on. Generation 0 means
    // nothing has been requested yet.
    std::atomic<uint32_t> m_request{0};
    // When the newest request was made: the steady clock in milliseconds, cut
    // to 32 bits. Differences stay right for 49 days.
    std::atomic<uint32_t> m_requestedAtMs{0};
    // The request words most recently delivered and abandoned.
    std::atomic<uint32_t> m_deliveredRequest{0};
    std::atomic<uint32_t> m_abandonedRequest{0};
    std::atomic<bool> m_stop{false};

    std::atomic<uint32_t> m_attempts{0};
    std::atomic<uint32_t> m_delivered{0};
    std::atomic<uint32_t> m_failed{0};
    std::atomic<uint32_t> m_abandoned{0};

    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::thread m_thread;
};

}  // namespace tplink
