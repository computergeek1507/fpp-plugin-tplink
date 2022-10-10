#pragma once

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
#include "log.h"

class TPLinkItem {
public:
    TPLinkItem(std::string const& ip, unsigned int startChannel );
    virtual ~TPLinkItem();

    std::string GetIPAddress() const { return m_ipAddress; }
    unsigned int GetStartChannel() const { return m_startChannel; }

    void EnableOutput() { m_unreachable = false; }

    virtual bool SendData(unsigned char *data) = 0;

    virtual std::string GetType() const = 0;
    virtual std::string GetConfigString() const = 0;

    std::string getInfo();

protected:
    std::string m_ipAddress;
    uint16_t m_port;
    unsigned int m_startChannel;
    unsigned int m_seqCount;

    std::atomic<bool> m_unreachable;

    std::string sendCmd(std::string const& cmd);

private:
    static void serializeUint32(char (&buf)[4], uint32_t val);
    static void encrypt(char *data, uint16_t length);
    static void encryptWithHeader(char *out, char *data, uint16_t length);
    static void decrypt(char* input, uint16_t length);
    static uint16_t sockConnect(char* out, const char *ip_add, int port, const char *cmd, uint16_t length);

};