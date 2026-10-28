#pragma once

#include <cstdint>
#include <string>

// The crypto side of KLAP, the local protocol newer Tapo (and some Kasa)
// devices speak over HTTP on port 80. Byte for byte the same as python-kasa's
// klaptransport.py. Nothing here depends on FPP or the network.
//
// Handshake: the client POSTs a random 16-byte local seed to /app/handshake1
// and gets back a 16-byte remote seed plus a hash proving the device knows the
// account. The client checks that hash against the credentials it has, then
// POSTs its own proof to /app/handshake2. Both sides then derive an AES-128
// key, an IV and a signing key from the two seeds and the auth hash.
namespace tplink {
namespace klap {

// Tapo devices use version 2 (SHA based); Kasa devices on KLAP use version 1
// (MD5 based). The device tells us which by the hash it sends in handshake1.
enum class Version { V1, V2 };

// md5(md5(user) + md5(pass)) for V1, sha256(sha1(user) + sha1(pass)) for V2.
std::string authHash(Version v, std::string const& username, std::string const& password);

// What the device sends after the remote seed in handshake1.
std::string handshake1Hash(Version v, std::string const& localSeed, std::string const& remoteSeed,
                           std::string const& authHash);

// What the client sends in handshake2.
std::string handshake2Hash(Version v, std::string const& localSeed, std::string const& remoteSeed,
                           std::string const& authHash);

// One session's AES state. Every request uses the next sequence number, which
// is both the last 4 bytes of the IV and the ?seq= the device checks.
class Cipher {
public:
    Cipher(std::string const& localSeed, std::string const& remoteSeed, std::string const& authHash);

    // Advances the sequence number and returns signature(32) + ciphertext.
    std::string encrypt(std::string const& plain, int32_t& seq);

    // Decrypts a reply to request `seq` (the 32-byte signature is skipped, as
    // python-kasa does). False if the padding or length is wrong.
    bool decrypt(std::string const& body, int32_t seq, std::string& plain) const;

private:
    std::string m_key;  // 16 bytes
    std::string m_iv;   // 12 bytes; the sequence number makes up the rest
    std::string m_sig;  // 28 bytes
    int32_t m_seq;
};

// Random bytes from the system's CSPRNG.
std::string randomBytes(size_t count);

std::string md5(std::string const& data);
std::string sha1(std::string const& data);
std::string sha256(std::string const& data);
std::string base64(std::string const& data);
// Decodes standard base64; false on malformed input.
bool base64Decode(std::string const& text, std::string& out);

}  // namespace klap
}  // namespace tplink
