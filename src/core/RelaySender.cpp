#include "RelaySender.h"

#include <algorithm>
#include <utility>

namespace tplink {

namespace {

using Clock = std::chrono::steady_clock;

// The steady clock in milliseconds, cut to 32 bits.
uint32_t steadyMs() {
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count());
}

}  // namespace

RelaySender::Timing RelaySender::defaultTiming() {
    return Timing{};
}

RelaySender::RelaySender(SendFn send, Timing timing, GiveUpFn onGiveUp)
    : m_send(std::move(send)), m_timing(timing), m_onGiveUp(std::move(onGiveUp)) {
    if (m_timing.firstRetry < std::chrono::milliseconds(1)) {
        m_timing.firstRetry = std::chrono::milliseconds(1);
    }
    if (m_timing.maxRetry < m_timing.firstRetry) {
        m_timing.maxRetry = m_timing.firstRetry;
    }
    // The 32-bit request time only measures up to 49 days.
    m_timing.giveUpAfter = std::clamp<std::chrono::milliseconds>(m_timing.giveUpAfter, std::chrono::milliseconds(0),
                                                                  std::chrono::hours(24));
}

RelaySender::~RelaySender() {
    stop();
}

void RelaySender::start() {
    if (m_thread.joinable()) {
        return;
    }
    m_stop.store(false);
    m_thread = std::thread(&RelaySender::run, this);
}

void RelaySender::request(bool on) {
    // The time goes in before the word, so the sender, which reads the word
    // first, never pairs a request with an older request's time.
    m_requestedAtMs.store(steadyMs());
    uint32_t current = m_request.load();
    uint32_t next = 0;
    do {
        uint32_t generation = (current >> 1) + 1;
        if (generation > 0x7fffffffu) {
            generation = 1;  // after two billion changes
        }
        next = (generation << 1) | (on ? 1u : 0u);
    } while (!m_request.compare_exchange_weak(current, next));
    wake();
}

void RelaySender::requestStop() {
    m_stop.store(true);
    wake();
}

void RelaySender::stop() {
    requestStop();
    if (m_thread.joinable() && m_thread.get_id() != std::this_thread::get_id()) {
        m_thread.join();
    }
}

bool RelaySender::pending() const {
    const uint32_t word = m_request.load();
    return word != m_deliveredRequest.load() && word != m_abandonedRequest.load();
}

bool RelaySender::idle() const {
    return !pending();
}

bool RelaySender::gaveUp() const {
    const uint32_t word = m_request.load();
    return word != 0 && word == m_abandonedRequest.load();
}

RelaySender::Stats RelaySender::stats() const {
    return Stats{m_attempts.load(), m_delivered.load(), m_failed.load(), m_abandoned.load()};
}

void RelaySender::wake() {
    // Holding the mutex for an instant closes the gap between the sender
    // checking for work and starting to wait. The sender never holds it across
    // I/O, so this never waits on the network.
    { std::lock_guard<std::mutex> lock(m_mutex); }
    m_wake.notify_one();
}

bool RelaySender::attempt(bool on) {
    m_attempts.fetch_add(1);
    bool ok = false;
    try {
        ok = m_send && m_send(on, m_stop);
    } catch (...) {
        ok = false;
    }
    (ok ? m_delivered : m_failed).fetch_add(1);
    return ok;
}

void RelaySender::abandon(std::unique_lock<std::mutex>& lock, uint32_t word, unsigned tries) {
    m_abandonedRequest.store(word);
    m_abandoned.fetch_add(1);
    if (!m_onGiveUp) {
        return;
    }
    lock.unlock();
    try {
        m_onGiveUp((word & 1u) != 0, tries, m_timing.giveUpAfter);
    } catch (...) {
    }
    lock.lock();
}

void RelaySender::run() {
    uint32_t current = 0;         // the request being worked on
    Clock::time_point giveUpAt;   // when retrying it stops
    auto retryDelay = m_timing.firstRetry;
    unsigned tries = 0;

    std::unique_lock<std::mutex> lock(m_mutex);
    for (;;) {
        m_wake.wait(lock, [this] { return m_stop.load() || pending(); });
        if (m_stop.load()) {
            return;
        }

        const uint32_t word = m_request.load();
        if (word != current) {
            // A newer request: a window of its own, measured from when it was
            // requested, and the first retry delay again. Its time is read
            // before the clock, so the difference can never run backwards.
            current = word;
            const uint32_t requestedAt = m_requestedAtMs.load();
            const auto sinceRequested = std::chrono::milliseconds(steadyMs() - requestedAt);
            giveUpAt = Clock::now() - sinceRequested + m_timing.giveUpAfter;
            retryDelay = m_timing.firstRetry;
            tries = 0;
        }

        lock.unlock();
        const bool ok = attempt((word & 1u) != 0);
        lock.lock();
        ++tries;

        if (ok) {
            m_deliveredRequest.store(word);
            continue;
        }

        // Wait before trying again, but never past the end of the window. A
        // newer request or a stop ends the wait at once.
        const bool interrupted = m_wake.wait_until(lock, std::min(Clock::now() + retryDelay, giveUpAt),
                                                   [this, word] { return m_stop.load() || m_request.load() != word; });
        if (interrupted) {
            continue;
        }
        if (Clock::now() >= giveUpAt) {
            abandon(lock, word, tries);
            continue;
        }
        retryDelay = std::min(retryDelay * 2, m_timing.maxRetry);
    }
}

}  // namespace tplink
