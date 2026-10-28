#include "Klap.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <memory>
#include <stdexcept>

namespace tplink {
namespace klap {

namespace {

std::string digest(EVP_MD const* md, std::string const& data) {
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    if (EVP_Digest(data.data(), data.size(), out, &length, md, nullptr) != 1) {
        throw std::runtime_error("digest failed");
    }
    return std::string(reinterpret_cast<char*>(out), length);
}

std::string packSeq(int32_t seq) {
    const uint32_t u = static_cast<uint32_t>(seq);
    std::string out(4, '\0');
    out[0] = static_cast<char>((u >> 24) & 0xFF);
    out[1] = static_cast<char>((u >> 16) & 0xFF);
    out[2] = static_cast<char>((u >> 8) & 0xFF);
    out[3] = static_cast<char>(u & 0xFF);
    return out;
}

struct CtxFree {
    void operator()(EVP_CIPHER_CTX* ctx) const { EVP_CIPHER_CTX_free(ctx); }
};

// AES-128-CBC with PKCS#7 padding, as python-kasa uses.
bool aes(bool encrypt, std::string const& key, std::string const& iv, std::string const& in, std::string& out) {
    std::unique_ptr<EVP_CIPHER_CTX, CtxFree> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return false;
    }
    auto const* k = reinterpret_cast<unsigned char const*>(key.data());
    auto const* v = reinterpret_cast<unsigned char const*>(iv.data());
    if (EVP_CipherInit_ex(ctx.get(), EVP_aes_128_cbc(), nullptr, k, v, encrypt ? 1 : 0) != 1) {
        return false;
    }
    out.assign(in.size() + 16, '\0');
    int written = 0;
    int finalLen = 0;
    auto* o = reinterpret_cast<unsigned char*>(&out[0]);
    if (EVP_CipherUpdate(ctx.get(), o, &written, reinterpret_cast<unsigned char const*>(in.data()),
                         static_cast<int>(in.size())) != 1) {
        return false;
    }
    if (EVP_CipherFinal_ex(ctx.get(), o + written, &finalLen) != 1) {
        return false;
    }
    out.resize(static_cast<size_t>(written + finalLen));
    return true;
}

}  // namespace

std::string md5(std::string const& data) { return digest(EVP_md5(), data); }
std::string sha1(std::string const& data) { return digest(EVP_sha1(), data); }
std::string sha256(std::string const& data) { return digest(EVP_sha256(), data); }

std::string base64(std::string const& data) {
    std::string out(4 * ((data.size() + 2) / 3) + 1, '\0');
    const int n = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&out[0]),
                                  reinterpret_cast<unsigned char const*>(data.data()), static_cast<int>(data.size()));
    out.resize(n > 0 ? static_cast<size_t>(n) : 0);
    return out;
}

bool base64Decode(std::string const& text, std::string& out) {
    out.clear();
    if (text.empty()) {
        return true;
    }
    if (text.size() % 4 != 0) {
        return false;
    }
    std::string buf(text.size() / 4 * 3, '\0');
    const int n = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(&buf[0]),
                                  reinterpret_cast<unsigned char const*>(text.data()), static_cast<int>(text.size()));
    if (n < 0) {
        return false;
    }
    // EVP_DecodeBlock counts padding as data; drop a byte per '='.
    size_t length = static_cast<size_t>(n);
    for (size_t i = text.size(); i-- > 0 && text[i] == '=' && length > 0;) {
        --length;
    }
    buf.resize(length);
    out.swap(buf);
    return true;
}

std::string randomBytes(size_t count) {
    std::string out(count, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char*>(&out[0]), static_cast<int>(count)) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    return out;
}

std::string authHash(Version v, std::string const& username, std::string const& password) {
    if (v == Version::V1) {
        return md5(md5(username) + md5(password));
    }
    return sha256(sha1(username) + sha1(password));
}

std::string handshake1Hash(Version v, std::string const& localSeed, std::string const& remoteSeed,
                           std::string const& auth) {
    if (v == Version::V1) {
        return sha256(localSeed + auth);
    }
    return sha256(localSeed + remoteSeed + auth);
}

std::string handshake2Hash(Version v, std::string const& localSeed, std::string const& remoteSeed,
                           std::string const& auth) {
    if (v == Version::V1) {
        return sha256(remoteSeed + auth);
    }
    return sha256(remoteSeed + localSeed + auth);
}

Cipher::Cipher(std::string const& localSeed, std::string const& remoteSeed, std::string const& auth) {
    const std::string seeds = localSeed + remoteSeed + auth;
    m_key = sha256("lsk" + seeds).substr(0, 16);
    const std::string fullIv = sha256("iv" + seeds);
    m_iv = fullIv.substr(0, 12);
    const auto* tail = reinterpret_cast<unsigned char const*>(fullIv.data() + 28);
    m_seq = static_cast<int32_t>((static_cast<uint32_t>(tail[0]) << 24) | (static_cast<uint32_t>(tail[1]) << 16) |
                                 (static_cast<uint32_t>(tail[2]) << 8) | static_cast<uint32_t>(tail[3]));
    m_sig = sha256("ldk" + seeds).substr(0, 28);
}

std::string Cipher::encrypt(std::string const& plain, int32_t& seq) {
    m_seq = static_cast<int32_t>(static_cast<uint32_t>(m_seq) + 1u);
    seq = m_seq;
    std::string cipherText;
    if (!aes(true, m_key, m_iv + packSeq(seq), plain, cipherText)) {
        throw std::runtime_error("AES encrypt failed");
    }
    return sha256(m_sig + packSeq(seq) + cipherText) + cipherText;
}

bool Cipher::decrypt(std::string const& body, int32_t seq, std::string& plain) const {
    if (body.size() < 32 + 16 || (body.size() - 32) % 16 != 0) {
        return false;
    }
    return aes(false, m_key, m_iv + packSeq(seq), body.substr(32), plain);
}

}  // namespace klap
}  // namespace tplink
