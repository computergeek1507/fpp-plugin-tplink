#pragma once

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

#include "common.h"
#include "log.h"

class TPLinkItem {
public:
    TPLinkItem(std::string const& ip, unsigned int startChannel );
    ~TPLinkItem();

    bool SendData( unsigned char *data);

    std::string setLightOnRGB( uint8_t r, uint8_t g, uint8_t b, int color_Temp = 0, int period = 0);
    std::string setLightOnHSV( int hue, int saturation, int brightness, int color_Temp = 0, int period = 0);
    std::string setLightOff();
    std::string setRelayOn(int plug_num);
    std::string setRelayOff(int plug_num);

    std::string GetIPAddress(){return m_ipAddress;}
    unsigned int GetStartChannel(){return m_startChannel;}

    void EnableOutput(){m_unreachable = false;}

private:
    std::string m_ipAddress;
    uint16_t m_port;
    unsigned int m_startChannel;
    unsigned int m_seqCount;

    uint8_t m_r;
    uint8_t m_g;
    uint8_t m_b;
    std::atomic<bool> m_unreachable;

    void outputData( uint8_t r ,uint8_t g ,uint8_t b );
    void RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV);

    static void serializeUint32(char (&buf)[4], uint32_t val);
    static void encrypt(char *data, uint16_t length);
    static void encryptWithHeader(char *out, char *data, uint16_t length);
    static void decrypt(char* input, uint16_t length);
    static uint16_t sockConnect(char* out, const char *ip_add, int port, const char *cmd, uint16_t length);

    std::string sendCmd(std::string cmd, int plug_num = 0);

    std::string getInfo();

    std::string setLedOff();
    std::string setLedOn();
};