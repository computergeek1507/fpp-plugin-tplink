// Tests for the native Tapo client (src/core/Klap.* and TapoClient.*), run
// against fake_tapo.py, whose KLAP side is python-kasa's own implementation.
// run_tapo_tests.sh starts the fake device and runs this.

#include "../src/core/Klap.h"
#include "../src/core/MiniJson.h"
#include "../src/core/TapoClient.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using tplink::json::Value;
namespace tapo = tplink::tapo;
namespace klap = tplink::klap;

static int g_failures = 0;
static int g_checks = 0;
static uint16_t g_port = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                        \
    } while (0)

static std::string hex(std::string const& s) {
    static char const* digits = "0123456789abcdef";
    std::string out;
    for (unsigned char c : s) {
        out += digits[c >> 4];
        out += digits[c & 15];
    }
    return out;
}

// --- talking to the fake device's /test endpoints -------------------------

static std::string http(std::string const& method, std::string const& path, std::string const& body = "") {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a;
    std::memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons(g_port);
    ::inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) != 0) {
        ::close(fd);
        return std::string();
    }
    std::string req = method + " " + path + " HTTP/1.1\r\nHost: x\r\nContent-Length: " + std::to_string(body.size()) +
                      "\r\nConnection: close\r\n\r\n" + body;
    ::send(fd, req.data(), req.size(), 0);
    std::string data;
    char buf[4096];
    ssize_t n;
    while ((n = ::recv(fd, buf, sizeof(buf), 0)) > 0) {
        data.append(buf, static_cast<size_t>(n));
    }
    ::close(fd);
    const auto p = data.find("\r\n\r\n");
    return p == std::string::npos ? std::string() : data.substr(p + 4);
}

static void resetDevice() { http("POST", "/test/reset"); }
static void setDevice(std::string const& json) { http("POST", "/test/set", json); }

static Value deviceState() {
    Value v;
    tplink::json::parse(http("GET", "/test/state"), v);
    return v;
}

static long long num(Value const& v, char const* key) {
    Value const* m = v.member(key);
    return m ? std::atoll(m->text.c_str()) : -999;
}

static bool flag(Value const* v) { return v != nullptr && v->text == "true"; }

static std::string const kUser = "user@example.com";
static std::string const kPass = "s3cret!$ \"quoted\"";

static std::unique_ptr<tapo::Client> client(std::string const& user = kUser, std::string const& pass = kPass) {
    return std::unique_ptr<tapo::Client>(new tapo::Client("127.0.0.1", user, pass, g_port));
}

// --- tests ------------------------------------------------------------------

static void testHashes() {
    std::puts("hashes match python-kasa");
    // Expected values printed by python-kasa (see run_tapo_tests.sh: the
    // script checks these against the installed python-kasa too).
    CHECK(hex(klap::authHash(klap::Version::V2, "a@b.c", "pw")) == std::getenv("EXPECT_V2_AUTH"));
    CHECK(hex(klap::authHash(klap::Version::V1, "a@b.c", "pw")) == std::getenv("EXPECT_V1_AUTH"));

    const std::string local(16, '\x01'), remote(16, '\x02');
    const std::string auth = klap::authHash(klap::Version::V2, "a@b.c", "pw");
    klap::Cipher cipher(local, remote, auth);
    int32_t seq = 0;
    const std::string enc = cipher.encrypt("{\"method\":\"get_device_info\"}", seq);
    CHECK(std::to_string(seq) == std::getenv("EXPECT_SEQ"));
    CHECK(hex(enc) == std::getenv("EXPECT_ENC"));
    std::string plain;
    CHECK(cipher.decrypt(enc, seq, plain));
    CHECK(plain == "{\"method\":\"get_device_info\"}");
}

static void testOnOff() {
    std::puts("plug on/off, with one handshake for several requests");
    resetDevice();
    auto c = client();
    Value r;
    std::string err;
    tapo::Options o;
    CHECK(c->call("set_device_info", "{\"device_on\":true}", r, o, &err));
    CHECK(flag(deviceState().member("device_on")));
    CHECK(c->call("set_device_info", "{\"device_on\":false}", r, o, &err));
    CHECK(!flag(deviceState().member("device_on")));
    CHECK(c->call("get_device_info", "", r, o, &err));
    CHECK(r.member("device_on") != nullptr && r.member("device_on")->text == "false");
    CHECK(num(deviceState(), "handshakes") == 1);
    if (!err.empty()) {
        std::printf("  (last error: %s)\n", err.c_str());
    }

    // The request envelope looks like python-kasa's.
    Value st = deviceState();
    Value const* req = st.member("requests")->element(0);
    CHECK(req->member("method")->asString() == "set_device_info");
    CHECK(req->member("terminal_uuid") != nullptr && !req->member("terminal_uuid")->asString().empty());
    CHECK(req->member("request_time_milis") != nullptr);
}

static void testWrongPassword() {
    std::puts("wrong password is reported, not retried forever");
    resetDevice();
    auto c = client(kUser, "nope");
    Value r;
    std::string err;
    CHECK(!c->call("set_device_info", "{\"device_on\":true}", r, tapo::Options(), &err));
    CHECK(err.find("did not accept") != std::string::npos);
    CHECK(num(deviceState(), "handshakes") == 1);
}

static void testFactoryDefaultAccount() {
    std::puts("a device still on the factory default account works");
    resetDevice();
    setDevice("{\"username\":\"test@tp-link.net\",\"password\":\"test\"}");
    auto c = client();
    Value r;
    std::string err;
    CHECK(c->call("set_device_info", "{\"device_on\":true}", r, tapo::Options(), &err));
    CHECK(flag(deviceState().member("device_on")));
}

static void testKasaKlapV1() {
    std::puts("Kasa devices on KLAP v1 (MD5 hashes) work too");
    resetDevice();
    setDevice("{\"version\":1}");
    auto c = client();
    Value r;
    std::string err;
    CHECK(c->call("set_device_info", "{\"device_on\":true}", r, tapo::Options(), &err));
    CHECK(flag(deviceState().member("device_on")));
}

static void testSessionRecovery() {
    std::puts("403, expired session and session error codes trigger one new handshake");
    resetDevice();
    auto c = client();
    Value r;
    std::string err;
    tapo::Options o;
    CHECK(c->call("get_device_info", "", r, o, &err));

    setDevice("{\"fail_next_403\":1}");
    CHECK(c->call("set_device_info", "{\"device_on\":true}", r, o, &err));
    CHECK(num(deviceState(), "handshakes") == 2);

    http("POST", "/test/expire");  // device forgot the session (e.g. rebooted)
    CHECK(c->call("set_device_info", "{\"device_on\":false}", r, o, &err));
    CHECK(num(deviceState(), "handshakes") == 3);

    setDevice("{\"fail_next_code\":-40401}");
    CHECK(c->call("get_device_info", "", r, o, &err));
    CHECK(num(deviceState(), "handshakes") == 4);

    // A device that keeps refusing gives up after one retry.
    setDevice("{\"fail_next_403\":5}");
    CHECK(!c->call("get_device_info", "", r, o, &err));
    CHECK(num(deviceState(), "handshakes") == 5);
}

static void testDeviceErrors() {
    std::puts("device error codes come back as failures");
    resetDevice();
    auto c = client();
    Value r;
    std::string err;
    CHECK(!c->call("no_such_method", "", r, tapo::Options(), &err));
    CHECK(err.find("-1002") != std::string::npos);
}

static void testStripOutlets() {
    std::puts("strip outlets are found by position and switched with control_child");
    resetDevice();
    auto c = client();
    Value list;
    std::string err;
    tapo::Options o;
    CHECK(c->call("get_child_device_list", "{\"start_index\":0}", list, o, &err));
    CHECK(tapo::childIdAt(list, 1) == "child-aaa");
    CHECK(tapo::childIdAt(list, 2) == "child-bbb");
    CHECK(tapo::childIdAt(list, 3) == "child-ccc");
    CHECK(tapo::childIdAt(list, 4).empty());

    Value r;
    CHECK(c->callChild("child-bbb", "set_device_info", "{\"device_on\":true}", r, o, &err));
    Value st = deviceState();
    Value const* children = st.member("children");
    CHECK(flag(children->member("child-bbb")->member("device_on")));
    CHECK(!flag(children->member("child-aaa")->member("device_on")));
    CHECK(!c->callChild("child-zzz", "set_device_info", "{\"device_on\":true}", r, o, &err));
    CHECK(err.find("-1008") != std::string::npos);
}

static void testChildListWithoutPositions() {
    std::puts("child list without position fields falls back to list order");
    Value v;
    tplink::json::parse("{\"child_device_list\":[{\"device_id\":\"x\"},{\"device_id\":\"y\"}]}", v);
    CHECK(tapo::childIdAt(v, 1) == "x");
    CHECK(tapo::childIdAt(v, 2) == "y");
    CHECK(tapo::childIdAt(v, 0).empty());
}

static void testChunkedReplies() {
    std::puts("chunked HTTP replies are read");
    resetDevice();
    setDevice("{\"chunked\":true}");
    auto c = client();
    Value r;
    std::string err;
    CHECK(c->call("set_device_info", "{\"device_on\":true}", r, tapo::Options(), &err));
    CHECK(flag(deviceState().member("device_on")));
}

static void testSharedClients() {
    std::puts("shared clients: one per device and account, safe across threads");
    resetDevice();
    auto a = tapo::Client::shared("127.0.0.1", kUser, kPass, g_port);
    auto b = tapo::Client::shared("127.0.0.1", kUser, kPass, g_port);
    auto other = tapo::Client::shared("127.0.0.1", kUser, "different", g_port);
    CHECK(a.get() == b.get());
    CHECK(a.get() != other.get());

    std::vector<std::thread> threads;
    std::atomic<int> ok{0};
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 10; ++i) {
                Value r;
                std::string err;
                const std::string p = ((t + i) % 2) ? "{\"device_on\":true}" : "{\"device_on\":false}";
                if (b->call("set_device_info", p, r, tapo::Options(), &err)) {
                    ++ok;
                }
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(ok == 40);
    CHECK(num(deviceState(), "handshakes") == 1);
}

static void testUnreachable() {
    std::puts("an address nobody answers on fails within the timeout");
    tapo::Client c("127.0.0.1", kUser, kPass, 1);  // nothing listens on port 1
    Value r;
    std::string err;
    tapo::Options o;
    o.connectTimeout = std::chrono::milliseconds(500);
    const auto start = std::chrono::steady_clock::now();
    CHECK(!c.call("get_device_info", "", r, o, &err));
    CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));
    CHECK(!err.empty());
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s PORT\n", argv[0]);
        return 2;
    }
    g_port = static_cast<uint16_t>(std::atoi(argv[1]));

    testHashes();
    testOnOff();
    testWrongPassword();
    testFactoryDefaultAccount();
    testKasaKlapV1();
    testSessionRecovery();
    testDeviceErrors();
    testStripOutlets();
    testChildListWithoutPositions();
    testChunkedReplies();
    testSharedClients();
    testUnreachable();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
