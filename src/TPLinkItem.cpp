#include "TPLinkItem.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <arpa/inet.h>  // for inet_pton
#include <netinet/in.h> // for sockaddr_in, htons
#include <stdint.h>     // for uint16_t, uint8_t, uint32_t
#include <stdio.h>      // for printf
#include <sys/socket.h> // for AF_INET, connect, send, socket, SOCK_STREAM
#include <unistd.h>     // for close, read
#include <string>       // for string
#include <cstring>      // for ??? memcpy, memset, strncpy


#include <iostream>
#include <istream>
#include <ostream>

TPLinkItem::TPLinkItem(std::string const& ip, unsigned int startChannel) :
    m_startChannel(startChannel),
    m_ipAddress(ip),
    m_r(0),
    m_g(0),
    m_b(0),
    m_unreachable(false),
    m_seqCount(0),
    m_port(9999)
{
}

TPLinkItem::~TPLinkItem()
{

}

bool TPLinkItem::SendData( unsigned char *data)
{
    try
    {
        if(m_unreachable){
			return false;
		}

        uint8_t r = data[m_startChannel - 1];
        uint8_t g = data[m_startChannel];
        uint8_t b = data[m_startChannel + 1];

        if(r == m_r && g == m_g && b == m_b) {
            if(m_seqCount < 1201) {
                ++ m_seqCount;
                return true;
            }
        }
        m_seqCount=0;
        m_r = r;
        m_g = g;
        m_b = b;

        std::thread t(&TPLinkItem::outputData, this, r, g, b );
        t.detach();
        //outputData(r, g, b );
        return true;
    }
    catch(std::exception ex)
    {
        m_unreachable = true;
        LogInfo(VB_PLUGIN, "Error %s \n",ex.what());
    }
    return false;
}

void TPLinkItem::outputData( uint8_t r ,uint8_t g ,uint8_t b )
{
    //{"smartlife.iot.smartbulb.lightingservice":{"transition_light_state":{"ignore_default":1,"transition_period":150,"mode":"normal","hue":120,"on_off":1,"saturation":65,"color_temp":0,"brightness":10}}}
    
	float h,si,sv,i,v;

    RGBtoHSIV(r/255,g/255,b/255,h,si,sv,i,v);
    
    int ih = (h);
    int isi = (si*100);
    int isv = (sv*100);
    int ii = (i*100);
    int iv = (v*100);
	
	const std::string cmd = "{\"smartlife.iot.smartbulb.lightingservice\":{\"transition_light_state\":{\"ignore_default\":1,\"transition_period\":0,\"mode\":\"normal\",\"hue\":" 
	+ std::to_string(ih) + ",\"on_off\":1,\"saturation\":" + std::to_string(isv) + ",\"color_temp\":0,\"brightness\":" + std::to_string(iv) + "}}}";
    sendCmd(cmd);
}

void TPLinkItem::RGBtoHSIV(float fR, float fG, float fB, float& fH, float& fSI, float& fSV,float& fI, float& fV) {
    float M  = std::max(std::max(fR, fG), fB);
    float m = std::min(std::min(fR, fG), fB);
    float c = M-m;
    fV = M;
    //fL = (1.0/2.0)*(M+m);
    fI = (1.0/3.0)*(fR+fG+fB);
  
    if(c==0) {
        fH = 0.0;
        fSI = 0.0;
    }
    else {
        if(M==fR) {
            fH = fmod(((fG-fB)/c), 6.0);
        }
        else if(M==fG) {
            fH = (fB-fR)/c + 2.0;
        }
        else if(M==fB) {
            fH = (fR-fG)/c + 4.0;
        }
        fH *=60.0;
        if(fI!=0) {
            fSI = 1.0 - (m/fI);
        }
        else {
            fSI = 0.0;
        }
    }

    fSV = M == 0 ? 0 : (M - m) / M;

    if(fH < 0.0)
        fH += 360.0;
}

void TPLinkItem::decrypt(char *input, uint16_t length)
{
    uint8_t key = 171;
    uint8_t next_key;
    for (uint16_t i = 0; i < length; i++)
    {
        next_key = input[i];
        input[i] = key ^ input[i];
        key = next_key;
    }
}

void TPLinkItem::encrypt(char *data, uint16_t length)
{
    uint8_t key = 171;
    for (uint16_t i = 0; i < length + 1; i++)
    {
        data[i] = key ^ data[i];
        key = data[i];
    }
}

void TPLinkItem::encryptWithHeader(char *out, char *data, uint16_t length)
{
    char serialized[4];
    serializeUint32(serialized, length);
    encrypt(data, length);
    std::memcpy(out, &serialized, 4);
    std::memcpy(out + 4, data, length);
}

std::string TPLinkItem::getInfo()
{
    const std::string cmd = "{\"system\":{\"get_sysinfo\":{}}}";
    return sendCmd(cmd);
}

std::string TPLinkItem::on()
{
    const std::string cmd = "{\"system\":{\"set_relay_state\":{\"state\":1}}}";
    return sendCmd(cmd);
}

std::string TPLinkItem::off()
{
    const std::string cmd = "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
    return sendCmd(cmd);
}

std::string TPLinkItem::setLedOff()
{
    const std::string cmd = "{\"system\":{\"set_led_off\":{\"off\":1}}}";
    return sendCmd(cmd);
}

std::string TPLinkItem::setLedOn()
{
    const std::string cmd = "{\"system\":{\"set_led_off\":{\"off\":0}}}";
    return sendCmd(cmd);
}

std::string TPLinkItem::sendCmd(std::string cmd)
{
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

uint16_t TPLinkItem::sockConnect(char *out, const char *ip_add, int port, const char *cmd, uint16_t length)
{

    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buf[2048] = {0};
    //  char buffer[2048] = {0};
    //    char buffer[2048] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return 0;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip_add, &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return 0;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return 0;
    }
    send(sock, cmd, length, 0);

    valread = read(sock, buf, 2048);
    close(sock);

    if (valread == 0)
    {
        printf("\nNo bytes read\n");
    }
    else
    {
        // buf + 4 strips 4 byte header
        // valread - 3 leaves 1 byte for terminating null character
        strncpy(out, buf + 4, valread - 3);
    }

    return valread;
}

