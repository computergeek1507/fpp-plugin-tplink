#include "TPLinkItem.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <arpa/inet.h>  // for inet_pton
#include <netinet/in.h> // for sockaddr_in, htons
#include <netinet/tcp.h>
#include <stdint.h>     // for uint16_t, uint8_t, uint32_t
#include <stdio.h>      // for printf
#include <sys/socket.h> // for AF_INET, connect, send, socket, SOCK_STREAM
#include <unistd.h>     // for close, read
#include <string>       // for string
#include <cstring>      // for ??? memcpy, memset, strncpy


#include <iostream>
#include <istream>
#include <ostream>

TPLinkItem::TPLinkItem(std::string const& ip, unsigned int startChannel ):
    BaseItem(ip,startChannel), m_port(9999)
{
}

TPLinkItem::~TPLinkItem() {

}

std::string TPLinkItem::getInfo() {
    const std::string cmd = "{\"system\":{\"get_sysinfo\":{}}}";
    return sendCmd(cmd);
}

void TPLinkItem::serializeUint32(char (&buf)[4], uint32_t val) {
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = val & 0xff;
}

void TPLinkItem::decrypt(char *input, uint16_t length) {
    uint8_t key = 171;
    uint8_t next_key;
    for (uint16_t i = 0; i < length; i++)
    {
        next_key = input[i];
        input[i] = key ^ input[i];
        key = next_key;
    }
}

void TPLinkItem::encrypt(char *data, uint16_t length) {
    uint8_t key = 171;
    for (uint16_t i = 0; i < length + 1; i++) {
        data[i] = key ^ data[i];
        key = data[i];
    }
}

void TPLinkItem::encryptWithHeader(char *out, char *data, uint16_t length) {
    char serialized[4];
    serializeUint32(serialized, length);
    encrypt(data, length);
    std::memcpy(out, &serialized, 4);
    std::memcpy(out + 4, data, length);
}

std::string TPLinkItem::sendCmd(std::string const& cmd) {
    try {
        char encrypted[cmd.length() + 4];
        encryptWithHeader(encrypted, const_cast<char *>(cmd.c_str()), cmd.length());
        char response[2048] = {0};

        uint16_t length = sockConnect(response, m_ipAddress.c_str(), m_port, encrypted, cmd.length() + 4);
        if (length > 0) {
            decrypt(response, length - 4);
        } else {
            return std::string("");
        }
        return std::string(response);
    }
    catch(std::exception const& ex) {
        LogInfo(VB_PLUGIN, "Error %s \n", ex.what());
    }
    return std::string("");
}

uint16_t TPLinkItem::sockConnect(char *out, const char *ip_add, int port, const char *cmd, uint16_t length) {
    if(m_issending)
    {
        return 0;
    }
    m_issending = true;
    //struct sockaddr_in address;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buf[2048] = {0};
    //  char buffer[2048] = {0};
    //    char buffer[2048] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        m_issending = false;
        return 0;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip_add, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        m_issending = false;
        return 0;
    }

    int synRetries = 2; // Send a total of 3 SYN packets => Timeout ~7s
    setsockopt(sock, IPPROTO_TCP, TCP_SYNCNT, &synRetries, sizeof(synRetries));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        m_issending = false;
        return 0;
    }
    send(sock, cmd, length, 0);

    int br = recv(sock, buf, 2048, 0);
    int dataLen = 0;
    int valread = 0;
    if (br > 4) {
        dataLen = ((int)buf[2] << 8) + (int)buf[3];
        valread = br;
        while (br >= 0 && (valread < (dataLen + 4))) {
            br = recv(sock, buf + valread, 2048 - valread, 0);
            if (br > 0) {
                valread += br;
            }
        }
    }
    close(sock);

    if (valread == 0) {
        printf("\nNo bytes read\n");
    } else {
        // buf + 4 strips 4 byte header
        // valread - 3 leaves 1 byte for terminating null character
        strncpy(out, buf + 4, valread - 3);
    }
    m_issending = false;
    return valread;
}

