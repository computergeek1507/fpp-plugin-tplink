// The socket side of the legacy Kasa protocol: one command and its reply on a
// fresh TCP connection, with deadlines and a cancel flag, so a silent plug can
// never hold the caller for long.

#include "KasaProtocol.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace tplink {
namespace kasa {

namespace {

constexpr uint32_t kMaxMessageBytes = 1u << 20;
// How often a wait on the socket stops to look at the cancel flag.
constexpr int kPollSliceMs = 20;

using Clock = std::chrono::steady_clock;

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

std::string describeErrno(char const* what, int err) {
    return std::string(what) + ": " + std::strerror(err);
}

// Waits until `fd` is ready for `events` or `deadline` passes, looking at
// `cancel` between short polls.
bool waitReady(int fd, short events, Clock::time_point deadline, std::atomic<bool> const* cancel,
               char const* what, std::string* error) {
    for (;;) {
        if (cancel != nullptr && cancel->load()) {
            return fail(error, std::string(what) + " cancelled");
        }
        const auto now = Clock::now();
        if (now >= deadline) {
            return fail(error, std::string(what) + " timed out");
        }
        const long long left =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count() + 1;
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

bool readExactly(int fd, char* buffer, size_t size, Clock::time_point deadline, std::atomic<bool> const* cancel,
                 std::string* error) {
    size_t got = 0;
    while (got < size) {
        const ssize_t n = ::recv(fd, buffer + got, size - got, 0);
        if (n > 0) {
            got += static_cast<size_t>(n);
            continue;
        }
        if (n == 0) {
            return fail(error, "the plug closed the connection before its reply was complete");
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (!waitReady(fd, POLLIN, deadline, cancel, "reply", error)) {
                return false;
            }
            continue;
        }
        return fail(error, describeErrno("recv", errno));
    }
    return true;
}

bool connectWithin(int fd, sockaddr_in const& address, QueryOptions const& options, std::string* error) {
    if (::connect(fd, reinterpret_cast<sockaddr const*>(&address), sizeof(address)) == 0) {
        return true;
    }
    if (errno != EINPROGRESS && errno != EINTR) {
        return fail(error, describeErrno("connect", errno));
    }
    if (!waitReady(fd, POLLOUT, Clock::now() + options.connectTimeout, options.cancel, "connect", error)) {
        return false;
    }
    int connectError = 0;
    socklen_t length = sizeof(connectError);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &connectError, &length) != 0) {
        connectError = errno;
    }
    return connectError == 0 || fail(error, describeErrno("connect", connectError));
}

}  // namespace

bool query(std::string const& host, uint16_t port, std::string const& json, std::string& reply,
           QueryOptions const& options, std::string* error) {
    reply.clear();
    if (json.size() > kMaxMessageBytes) {
        return fail(error, "command too large");
    }

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        return fail(error, "not an IPv4 address: '" + host + "'");
    }

    ScopedFd sock(::socket(AF_INET, SOCK_STREAM, 0));
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

    if (!connectWithin(sock.get(), address, options, error)) {
        return false;
    }

    const auto deadline = Clock::now() + options.ioTimeout;
    if (!writeAll(sock.get(), frame(json), deadline, options.cancel, error)) {
        return false;
    }

    char header[4];
    if (!readExactly(sock.get(), header, sizeof(header), deadline, options.cancel, error)) {
        return false;
    }
    const uint32_t length = (static_cast<uint32_t>(static_cast<uint8_t>(header[0])) << 24) |
                            (static_cast<uint32_t>(static_cast<uint8_t>(header[1])) << 16) |
                            (static_cast<uint32_t>(static_cast<uint8_t>(header[2])) << 8) |
                            static_cast<uint32_t>(static_cast<uint8_t>(header[3]));
    if (length > kMaxMessageBytes) {
        return fail(error, "reply too large");
    }
    std::string cipher(length, '\0');
    if (length > 0 && !readExactly(sock.get(), &cipher[0], length, deadline, options.cancel, error)) {
        return false;
    }
    reply = decrypt(cipher);
    return true;
}

}  // namespace kasa
}  // namespace tplink
