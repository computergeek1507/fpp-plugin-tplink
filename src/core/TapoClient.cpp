// KLAP over plain HTTP/1.1, one request per TCP connection, with deadlines and
// a cancel flag so a silent device can never hold the caller for long. The
// session (key, sequence number, cookie) outlives the connections and is reused
// until it expires or the device turns it down.

#include "TapoClient.h"

#include "Klap.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <tuple>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace tplink {
namespace tapo {

namespace {

using Clock = std::chrono::steady_clock;

constexpr size_t kMaxReplyBytes = 1u << 20;
constexpr int kPollSliceMs = 20;
// Renew a session this long before the device's TIMEOUT says it ends, because
// the device's clock is not always right (python-kasa does the same).
constexpr long kSessionBufferSeconds = 20 * 60;
constexpr long kDefaultSessionSeconds = 24 * 60 * 60;

class ScopedFd {
public:
    explicit ScopedFd(int fd) : m_fd(fd) {}
    ~ScopedFd() {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
    }
    ScopedFd(ScopedFd const&) = delete;
    ScopedFd& operator=(ScopedFd const&) = delete;
    int get() const { return m_fd; }

private:
    int m_fd;
};

bool fail(std::string* error, std::string const& message) {
    if (error != nullptr) {
        *error = message;
    }
    return false;
}

std::string describeErrno(char const* what, int err) { return std::string(what) + ": " + std::strerror(err); }

bool waitReady(int fd, short events, Clock::time_point deadline, std::atomic<bool> const* cancel, char const* what,
               std::string* error) {
    for (;;) {
        if (cancel != nullptr && cancel->load()) {
            return fail(error, std::string(what) + " cancelled");
        }
        const auto now = Clock::now();
        if (now >= deadline) {
            return fail(error, std::string(what) + " timed out");
        }
        const long long left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count() + 1;
        pollfd entry;
        entry.fd = fd;
        entry.events = events;
        entry.revents = 0;
        const int ready = ::poll(&entry, 1, static_cast<int>(std::min<long long>(left, kPollSliceMs)));
        if (ready > 0) {
            return true;
        }
        if (ready < 0 && errno != EINTR) {
            return fail(error, describeErrno("poll", errno));
        }
    }
}

bool connectTo(std::string const& host, uint16_t port, ScopedFd& sock, Options const& options, std::string* error) {
    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        return fail(error, "not an IPv4 address: '" + host + "'");
    }
    if (sock.get() < 0) {
        return fail(error, describeErrno("socket", errno));
    }
    const int flags = ::fcntl(sock.get(), F_GETFL, 0);
    if (flags < 0 || ::fcntl(sock.get(), F_SETFL, flags | O_NONBLOCK) < 0) {
        return fail(error, describeErrno("fcntl", errno));
    }
#ifdef SO_NOSIGPIPE
    const int one = 1;
    ::setsockopt(sock.get(), SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    if (::connect(sock.get(), reinterpret_cast<sockaddr const*>(&address), sizeof(address)) == 0) {
        return true;
    }
    if (errno != EINPROGRESS && errno != EINTR) {
        return fail(error, describeErrno("connect", errno));
    }
    if (!waitReady(sock.get(), POLLOUT, Clock::now() + options.connectTimeout, options.cancel, "connect", error)) {
        return false;
    }
    int connectError = 0;
    socklen_t length = sizeof(connectError);
    if (::getsockopt(sock.get(), SOL_SOCKET, SO_ERROR, &connectError, &length) != 0) {
        connectError = errno;
    }
    return connectError == 0 || fail(error, describeErrno("connect", connectError));
}

bool writeAll(int fd, std::string const& data, Clock::time_point deadline, std::atomic<bool> const* cancel,
              std::string* error) {
    size_t sent = 0;
    while (sent < data.size()) {
#ifdef MSG_NOSIGNAL
        const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
#else
        const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, 0);
#endif
        if (n > 0) {
            sent += static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!waitReady(fd, POLLOUT, deadline, cancel, "send", error)) {
                return false;
            }
            continue;
        }
        return fail(error, describeErrno("send", n < 0 ? errno : EPIPE));
    }
    return true;
}

std::string lower(std::string s) {
    for (auto& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string trim(std::string const& s) {
    const auto b = s.find_first_not_of(" \t");
    if (b == std::string::npos) {
        return std::string();
    }
    const auto e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

// Parses "a=1; b=2" (or "a=1;b=2") into the cookies we care about.
void readCookies(std::string const& header, std::string& session, long& timeout) {
    size_t start = 0;
    while (start <= header.size()) {
        size_t end = header.find(';', start);
        if (end == std::string::npos) {
            end = header.size();
        }
        const std::string part = trim(header.substr(start, end - start));
        const auto eq = part.find('=');
        if (eq != std::string::npos) {
            const std::string name = part.substr(0, eq);
            const std::string value = part.substr(eq + 1);
            if (name == "TP_SESSIONID") {
                session = value;
            } else if (name == "TIMEOUT") {
                timeout = std::strtol(value.c_str(), nullptr, 10);
            }
        }
        start = end + 1;
    }
}

bool decodeChunked(std::string const& in, std::string& out) {
    out.clear();
    size_t pos = 0;
    for (;;) {
        const auto lineEnd = in.find("\r\n", pos);
        if (lineEnd == std::string::npos) {
            return false;
        }
        const unsigned long size = std::strtoul(in.substr(pos, lineEnd - pos).c_str(), nullptr, 16);
        pos = lineEnd + 2;
        if (size == 0) {
            return true;
        }
        if (pos + size > in.size()) {
            return false;
        }
        out.append(in, pos, size);
        pos += size + 2;
    }
}

bool isNumber(json::Value const* v) { return v != nullptr && v->type == json::Value::Type::Number; }

long long numberOf(json::Value const* v) { return isNumber(v) ? std::strtoll(v->text.c_str(), nullptr, 10) : 0; }

// SMART errors that mean the session is no good and a new handshake may fix it.
bool sessionError(long long code) {
    switch (code) {
    case 9999:    // SESSION_TIMEOUT_ERROR
    case -40401:  // SESSION_EXPIRED
    case -1005:   // AES_DECODE_FAIL_ERROR
    case -1501:   // LOGIN_ERROR
    case 1100:    // HAND_SHAKE_FAILED_ERROR
    case 1111:    // LOGIN_FAILED_ERROR
    case 1003:    // TRANSPORT_UNKNOWN_CREDENTIALS_ERROR
        return true;
    default:
        return false;
    }
}

std::string newTerminalUuid() { return klap::base64(klap::md5(klap::randomBytes(16))); }

long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace

std::string quote(std::string const& s) {
    std::string out = "\"";
    for (char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                out += buf;
            } else {
                out += c;
            }
        }
    }
    out += "\"";
    return out;
}

std::string toJson(json::Value const& value) {
    switch (value.type) {
    case json::Value::Type::Null:
        return "null";
    case json::Value::Type::Boolean:
    case json::Value::Type::Number:
        return value.text;
    case json::Value::Type::String:
        return quote(value.text);
    case json::Value::Type::Array: {
        std::string out = "[";
        for (size_t i = 0; i < value.values.size(); ++i) {
            out += (i ? "," : "") + toJson(value.values[i]);
        }
        return out + "]";
    }
    case json::Value::Type::Object: {
        std::string out = "{";
        for (size_t i = 0; i < value.values.size() && i < value.keys.size(); ++i) {
            out += (i ? "," : "") + quote(value.keys[i]) + ":" + toJson(value.values[i]);
        }
        return out + "}";
    }
    }
    return "null";
}

std::string requestJson(std::string const& method, std::string const& paramsJson, std::string const& terminalUuid,
                        long long timeMs) {
    std::string out = "{\"method\":" + quote(method);
    if (!paramsJson.empty()) {
        out += ",\"params\":" + paramsJson;
    }
    out += ",\"request_time_milis\":" + std::to_string(timeMs);
    out += ",\"terminal_uuid\":" + quote(terminalUuid) + "}";
    return out;
}

std::string childIdAt(json::Value const& result, int position) {
    json::Value const* list = result.member("child_device_list");
    if (list == nullptr || list->type != json::Value::Type::Array || position < 1) {
        return std::string();
    }
    for (auto const& child : list->values) {
        json::Value const* pos = child.member("position");
        if (isNumber(pos) && numberOf(pos) == position) {
            json::Value const* id = child.member("device_id");
            return id != nullptr ? id->asString() : std::string();
        }
    }
    json::Value const* child = list->element(static_cast<size_t>(position - 1));
    if (child == nullptr || child->member("position") != nullptr) {
        return std::string();
    }
    json::Value const* id = child->member("device_id");
    return id != nullptr ? id->asString() : std::string();
}

std::string describeError(long long code) {
    std::string text = "device error " + std::to_string(code);
    switch (code) {
    case -1008:
        text += " (bad parameters; the device may not support this command)";
        break;
    case -1002:
        text += " (unknown method; the device does not support this command)";
        break;
    case -1501:
    case 1111:
    case 1003:
        text += " (login failed; check the Tapo email and password)";
        break;
    case 9999:
    case -40401:
        text += " (session expired)";
        break;
    default:
        break;
    }
    return text;
}

Credentials Credentials::fromPassword(std::string const& email, std::string const& password) {
    Credentials c;
    c.authV1 = klap::authHash(klap::Version::V1, email, password);
    c.authV2 = klap::authHash(klap::Version::V2, email, password);
    return c;
}

Credentials Credentials::fromBase64(std::string const& authV1, std::string const& authV2) {
    Credentials c;
    if (!klap::base64Decode(authV1, c.authV1) || c.authV1.size() != 16) {
        c.authV1.clear();
    }
    if (!klap::base64Decode(authV2, c.authV2) || c.authV2.size() != 32) {
        c.authV2.clear();
    }
    return c;
}

Client::Client(std::string const& host, Credentials const& credentials, uint16_t port) :
    m_host(host), m_port(port), m_credentials(credentials), m_terminalUuid(newTerminalUuid()) {}

Client::Client(std::string const& host, std::string const& username, std::string const& password, uint16_t port) :
    Client(host, Credentials::fromPassword(username, password), port) {}

Client::~Client() = default;

std::shared_ptr<Client> Client::shared(std::string const& host, Credentials const& credentials, uint16_t port) {
    static std::mutex registryMutex;
    static std::map<std::tuple<std::string, uint16_t, std::string, std::string>, std::weak_ptr<Client>> registry;

    std::lock_guard<std::mutex> lock(registryMutex);
    const auto key = std::make_tuple(host, port, credentials.authV1, credentials.authV2);
    auto it = registry.find(key);
    if (it != registry.end()) {
        if (auto existing = it->second.lock()) {
            return existing;
        }
    }
    // Forget clients nobody holds any more.
    for (auto i = registry.begin(); i != registry.end();) {
        i = i->second.expired() ? registry.erase(i) : std::next(i);
    }
    auto client = std::make_shared<Client>(host, credentials, port);
    registry[key] = client;
    return client;
}

void Client::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cipher.reset();
    m_sessionCookie.clear();
}

bool Client::post(std::string const& path, std::string const& body, HttpReply& reply, Options const& options,
                  std::string* error) const {
    reply = HttpReply();
    ScopedFd sock(::socket(AF_INET, SOCK_STREAM, 0));
    if (!connectTo(m_host, m_port, sock, options, error)) {
        return false;
    }

    std::string request = "POST " + path + " HTTP/1.1\r\n";
    request += "Host: " + m_host + (m_port != 80 ? ":" + std::to_string(m_port) : std::string()) + "\r\n";
    request += "Accept: */*\r\n";
    request += "Content-Type: application/octet-stream\r\n";
    request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    if (!m_sessionCookie.empty()) {
        request += "Cookie: TP_SESSIONID=" + m_sessionCookie + "\r\n";
    }
    request += "Connection: close\r\n\r\n";
    request += body;

    const auto deadline = Clock::now() + options.ioTimeout;
    if (!writeAll(sock.get(), request, deadline, options.cancel, error)) {
        return false;
    }

    // Read until the headers are in, then until Content-Length bytes of body
    // (or the device closes the connection).
    std::string data;
    size_t headerEnd = std::string::npos;
    long long contentLength = -1;
    bool chunked = false;
    char buffer[4096];
    for (;;) {
        if (headerEnd != std::string::npos && contentLength >= 0 &&
            data.size() >= headerEnd + 4 + static_cast<size_t>(contentLength)) {
            break;
        }
        const ssize_t n = ::recv(sock.get(), buffer, sizeof(buffer), 0);
        if (n > 0) {
            data.append(buffer, static_cast<size_t>(n));
            if (data.size() > kMaxReplyBytes) {
                return fail(error, "reply too large");
            }
            if (headerEnd == std::string::npos) {
                headerEnd = data.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    const std::string head = data.substr(0, headerEnd);
                    size_t lineStart = head.find("\r\n");
                    const std::string statusLine = head.substr(0, lineStart);
                    const auto sp = statusLine.find(' ');
                    if (statusLine.compare(0, 5, "HTTP/") != 0 || sp == std::string::npos) {
                        return fail(error, "not an HTTP reply");
                    }
                    reply.status = std::atoi(statusLine.c_str() + sp + 1);
                    while (lineStart != std::string::npos) {
                        lineStart += 2;
                        const size_t lineEnd = head.find("\r\n", lineStart);
                        const std::string line = head.substr(lineStart, lineEnd == std::string::npos
                                                                            ? std::string::npos
                                                                            : lineEnd - lineStart);
                        const auto colon = line.find(':');
                        if (colon != std::string::npos) {
                            const std::string name = lower(trim(line.substr(0, colon)));
                            const std::string value = trim(line.substr(colon + 1));
                            if (name == "content-length") {
                                contentLength = std::strtoll(value.c_str(), nullptr, 10);
                            } else if (name == "transfer-encoding" && lower(value).find("chunked") != std::string::npos) {
                                chunked = true;
                            } else if (name == "set-cookie") {
                                readCookies(value, reply.sessionCookie, reply.timeoutSeconds);
                            }
                        }
                        lineStart = lineEnd;
                    }
                    if (chunked) {
                        contentLength = -1;
                    }
                }
            }
            continue;
        }
        if (n == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (!waitReady(sock.get(), POLLIN, deadline, options.cancel, "reply", error)) {
                return false;
            }
            continue;
        }
        return fail(error, describeErrno("recv", errno));
    }

    if (headerEnd == std::string::npos) {
        return fail(error, "the device closed the connection before replying");
    }
    std::string rawBody = data.substr(headerEnd + 4);
    if (chunked) {
        if (!decodeChunked(rawBody, reply.body)) {
            return fail(error, "bad chunked reply");
        }
    } else if (contentLength >= 0) {
        if (rawBody.size() < static_cast<size_t>(contentLength)) {
            return fail(error, "the device closed the connection before its reply was complete");
        }
        reply.body = rawBody.substr(0, static_cast<size_t>(contentLength));
    } else {
        reply.body = rawBody;
    }
    return true;
}

bool Client::handshake(Options const& options, std::string* error) {
    m_cipher.reset();
    m_sessionCookie.clear();

    const std::string localSeed = klap::randomBytes(16);
    HttpReply reply;
    if (!post("/app/handshake1", localSeed, reply, options, error)) {
        return false;
    }
    if (reply.status != 200) {
        return fail(error, "handshake1 got HTTP " + std::to_string(reply.status) +
                               "; the device may not speak KLAP (older Tapo firmware uses a different protocol,"
                               " update the firmware) or third-party control is off in the Tapo app");
    }
    if (reply.body.size() != 48) {
        return fail(error, "handshake1 reply was " + std::to_string(reply.body.size()) + " bytes, expected 48");
    }
    const std::string remoteSeed = reply.body.substr(0, 16);
    const std::string serverHash = reply.body.substr(16);

    // Which account and protocol version the device answered for, in the order
    // python-kasa tries them: ours, then the factory defaults, then blank.
    std::vector<Credentials> candidates;
    candidates.push_back(m_credentials);
    candidates.push_back(Credentials::fromPassword("test@tp-link.net", "test"));       // Tapo factory default
    candidates.push_back(Credentials::fromPassword("kasa@tp-link.net", "kasaSetup"));  // Kasa factory default
    candidates.push_back(Credentials::fromPassword("", ""));

    std::string authHash;
    klap::Version version = klap::Version::V2;
    bool matched = false;
    for (auto const& c : candidates) {
        for (klap::Version v : {klap::Version::V2, klap::Version::V1}) {
            std::string const& hash = v == klap::Version::V2 ? c.authV2 : c.authV1;
            if (!hash.empty() && klap::handshake1Hash(v, localSeed, remoteSeed, hash) == serverHash) {
                authHash = hash;
                version = v;
                matched = true;
                break;
            }
        }
        if (matched) {
            break;
        }
    }
    if (!matched) {
        return fail(error,
                    "the device did not accept the Tapo email/password (both are case-sensitive). "
                    "If they are right, turn on Third-Party Compatibility in the Tapo app");
    }

    m_sessionCookie = reply.sessionCookie;
    const long timeout = reply.timeoutSeconds > 0 ? reply.timeoutSeconds : kDefaultSessionSeconds;

    HttpReply reply2;
    if (!post("/app/handshake2", klap::handshake2Hash(version, localSeed, remoteSeed, authHash), reply2, options,
              error)) {
        m_sessionCookie.clear();
        return false;
    }
    if (reply2.status != 200) {
        m_sessionCookie.clear();
        return fail(error, "handshake2 got HTTP " + std::to_string(reply2.status));
    }

    m_cipher.reset(new klap::Cipher(localSeed, remoteSeed, authHash));
    m_sessionExpires = Clock::now() + std::chrono::seconds(std::max(60L, timeout - kSessionBufferSeconds));
    return true;
}

bool Client::send(std::string const& json, std::string& replyText, bool& sessionRejected, Options const& options,
                  std::string* error) {
    sessionRejected = false;
    int32_t seq = 0;
    const std::string body = m_cipher->encrypt(json, seq);
    HttpReply reply;
    if (!post("/app/request?seq=" + std::to_string(seq), body, reply, options, error)) {
        return false;
    }
    if (reply.status == 403) {
        sessionRejected = true;
        return fail(error, "the device turned the session down (HTTP 403)");
    }
    if (reply.status != 200) {
        return fail(error, "request got HTTP " + std::to_string(reply.status));
    }
    if (!m_cipher->decrypt(reply.body, seq, replyText)) {
        sessionRejected = true;
        return fail(error, "could not decrypt the reply");
    }
    return true;
}

bool Client::call(std::string const& method, std::string const& paramsJson, json::Value& result,
                  Options const& options, std::string* error) {
    std::lock_guard<std::mutex> lock(m_mutex);
    result = json::Value();
    const std::string request = requestJson(method, paramsJson, m_terminalUuid, nowMs());

    for (int attempt = 0; attempt < 2; ++attempt) {
        const bool freshSession = !m_cipher || Clock::now() >= m_sessionExpires;
        if (freshSession && !handshake(options, error)) {
            return false;
        }
        std::string replyText;
        bool rejected = false;
        if (!send(request, replyText, rejected, options, error)) {
            if (rejected && attempt == 0) {
                m_cipher.reset();
                continue;
            }
            if (!rejected) {
                // A dropped connection can leave the device a sequence number
                // ahead or behind; start clean next time.
                m_cipher.reset();
            }
            return false;
        }
        json::Value reply;
        if (!json::parse(replyText, reply) || reply.type != json::Value::Type::Object) {
            return fail(error, "reply is not JSON: " + replyText.substr(0, 200));
        }
        const long long code = numberOf(reply.member("error_code"));
        if (code != 0) {
            if (sessionError(code) && attempt == 0) {
                m_cipher.reset();
                continue;
            }
            return fail(error, describeError(code));
        }
        if (json::Value const* r = reply.member("result")) {
            result = *r;
        }
        return true;
    }
    return fail(error, "the device turned the session down twice");
}

bool Client::callChild(std::string const& childId, std::string const& method, std::string const& paramsJson,
                       json::Value& result, Options const& options, std::string* error) {
    std::string inner = "{\"method\":" + quote(method);
    if (!paramsJson.empty()) {
        inner += ",\"params\":" + paramsJson;
    }
    inner += "}";
    const std::string params = "{\"device_id\":" + quote(childId) + ",\"requestData\":" + inner + "}";

    json::Value outer;
    if (!call("control_child", params, outer, options, error)) {
        return false;
    }
    result = json::Value();
    json::Value const* data = outer.member("responseData");
    if (data == nullptr) {
        return true;
    }
    const long long code = numberOf(data->member("error_code"));
    if (code != 0) {
        return fail(error, "outlet: " + describeError(code));
    }
    if (json::Value const* r = data->member("result")) {
        result = *r;
    }
    return true;
}

}  // namespace tapo
}  // namespace tplink
