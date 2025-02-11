#pragma once

#include "BaseItem.h"

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
#include "log.h"

class TPLinkItem  : virtual public BaseItem {
public:
    TPLinkItem(std::string const& ip, unsigned int startChannel );
    virtual ~TPLinkItem();

    std::string getInfo();

protected:
    std::string sendCmd(std::string const& cmd);

private:
    static void serializeUint32(char (&buf)[4], uint32_t val);
    static void encrypt(char *data, uint16_t length);
    static void encryptWithHeader(char *out, char *data, uint16_t length);
    static void decrypt(char* input, uint16_t length);
    uint16_t sockConnect(char* out, const char *ip_add, int port, const char *cmd, uint16_t length);

};