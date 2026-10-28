#pragma once

#include "MiniJson.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace tplink {
namespace klap {
class Cipher;
}

// A native client for TP-Link's KLAP transport (HTTP on port 80): Tapo plugs,
// strips and bulbs and newer Kasa devices with "SMART" JSON requests (call()),
// and Kasa devices that moved their old "IOT" JSON onto KLAP (request()).
// Nothing here depends on FPP.
namespace tapo {

// What the client authenticates with: the KLAP auth hashes, never the
// password itself. v2 is what Tapo devices check, v1 what Kasa devices on KLAP
// check. Either may be empty.
struct Credentials {
    std::string authV1;  // md5(md5(email) + md5(password)), 16 bytes
    std::string authV2;  // sha256(sha1(email) + sha1(password)), 32 bytes

    static Credentials fromPassword(std::string const& email, std::string const& password);
    // From the base64 text the settings page stores. Bad base64 gives an empty hash.
    static Credentials fromBase64(std::string const& authV1, std::string const& authV2);
    bool empty() const { return authV1.empty() && authV2.empty(); }
};

struct Options {
    std::chrono::milliseconds connectTimeout{3000};
    // How long one HTTP exchange (request and whole reply) may take.
    std::chrono::milliseconds ioTimeout{5000};
    // When set, the call gives up within about 20 ms of this turning true.
    std::atomic<bool> const* cancel = nullptr;
};

class Client {
public:
    Client(std::string const& host, Credentials const& credentials, uint16_t port = 80);
    Client(std::string const& host, std::string const& username, std::string const& password, uint16_t port = 80);
    ~Client();

    Client(Client const&) = delete;
    Client& operator=(Client const&) = delete;

    // One client per (host, port, credentials), shared by everything in the
    // process, so the plugin's devices and its commands reuse one session.
    static std::shared_ptr<Client> shared(std::string const& host, Credentials const& credentials,
                                          uint16_t port = 80);

    // Sends {"method": method, "params": params} and gives back the reply's
    // "result" (null when the reply has none). `paramsJson` is a JSON object's
    // text, or empty for no params. Handshakes when there is no session or it
    // has expired, and once more if the device turns the session down.
    // Thread safe; calls on one client run one at a time.
    bool call(std::string const& method, std::string const& paramsJson, json::Value& result,
              Options const& options, std::string* error = nullptr);

    // Sends `json` as it is and gives back the device's reply text, with no
    // SMART envelope or error_code check: for Kasa devices that speak the old
    // IOT commands ({"system":{...}}) over KLAP. Same session handling as call().
    bool request(std::string const& json, std::string& reply, Options const& options,
                 std::string* error = nullptr);

    // call() wrapped in control_child, for outlet `childId` of a strip. Gives
    // back the child's own result.
    bool callChild(std::string const& childId, std::string const& method, std::string const& paramsJson,
                   json::Value& result, Options const& options, std::string* error = nullptr);

    // Drops the session so the next call handshakes again.
    void reset();

    std::string const& host() const { return m_host; }

private:
    struct HttpReply {
        int status = 0;
        std::string body;
        std::string sessionCookie;  // TP_SESSIONID, when the reply set one
        long timeoutSeconds = 0;    // TIMEOUT cookie, when the reply set one
    };

    bool post(std::string const& path, std::string const& body, HttpReply& reply, Options const& options,
              std::string* error) const;
    bool handshake(Options const& options, std::string* error);
    bool send(std::string const& json, std::string& reply, bool& sessionRejected, Options const& options,
              std::string* error);
    // Handshakes when needed and sends; retries once on a refused session.
    // Takes m_mutex's lock from the caller.
    bool exchange(std::string const& json, std::string& reply, Options const& options, std::string* error);

    std::string m_host;
    uint16_t m_port;
    Credentials m_credentials;
    std::string m_terminalUuid;

    std::mutex m_mutex;
    std::unique_ptr<klap::Cipher> m_cipher;
    std::string m_sessionCookie;
    std::chrono::steady_clock::time_point m_sessionExpires;
};

// The full request text: method, optional params, request_time_milis and
// terminal_uuid, as python-kasa sends them.
std::string requestJson(std::string const& method, std::string const& paramsJson, std::string const& terminalUuid,
                        long long timeMs);

// JSON string literal for `s`, quotes included.
std::string quote(std::string const& s);

// Writes a parsed value back out as compact JSON.
std::string toJson(json::Value const& value);

// From a get_child_device_list result: the device_id of the outlet at
// `position` (counting from 1). Uses the list's own "position" fields, and
// falls back to list order when the device does not send them.
std::string childIdAt(json::Value const& childListResult, int position);

// Text for a non-zero SMART error code, with a hint for the common ones.
std::string describeError(long long code);

}  // namespace tapo
}  // namespace tplink
