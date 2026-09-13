#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

// The legacy Kasa protocol that TP-Link plugs and strips speak on TCP 9999.
// Nothing here depends on FPP.
namespace tplink {
namespace kasa {

constexpr uint16_t kDefaultPort = 9999;

// XOR "autokey" cipher: the key starts at 171 and each ciphertext byte is the
// key for the next.
std::string encrypt(std::string const& plain);
std::string decrypt(std::string const& cipher);

// A request as sent on the wire: a 4-byte big-endian length, then encrypt(json).
std::string frame(std::string const& json);

struct QueryOptions {
    // How long to wait for the TCP connection. About the three SYNs (7 s) the
    // plugin allowed before.
    std::chrono::milliseconds connectTimeout{7000};
    // How long sending the request and reading the whole reply may take.
    std::chrono::milliseconds ioTimeout{5000};
    // When set, the exchange gives up within about 20 ms of this turning true.
    std::atomic<bool> const* cancel = nullptr;
};

// Sends one command on a fresh connection and reads the plug's whole reply (any
// length up to 1 MiB), decrypted into `reply`. `host` must be an IPv4 address.
// Returns false on any failure, with the reason in `error` when it is given.
bool query(std::string const& host, uint16_t port, std::string const& json, std::string& reply,
           QueryOptions const& options, std::string* error = nullptr);

// {"system":{"get_sysinfo":{}}}, or the form that asks for children, for a
// strip outlet (plugNumber other than 0).
std::string sysinfoCommand(int plugNumber);

// {"system":{"set_relay_state":{"state":1}}}, or state 0.
std::string relayStateCommand(bool on);

// Addresses a command to outlet `plugNumber` of a strip by adding
// {"context":{"child_ids":["<childId>"]}}. Plug number 0 (a single-outlet
// plug) gets the command back unchanged.
std::string addressedCommand(std::string const& command, int plugNumber, std::string const& childId);

// From a get_sysinfo reply: the device id for plug number 0, otherwise the id of
// child `plugNumber` (counting from 1). Empty when the reply has no such id.
std::string idFromSysinfo(std::string const& reply, int plugNumber);

}  // namespace kasa
}  // namespace tplink
