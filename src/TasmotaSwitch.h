#pragma once

#include "BaseSwitch.h"

#include <curl/curl.h>

class TasmotaSwitch : public BaseSwitch{
public:
    TasmotaSwitch(std::string const& ip, unsigned int startChannel, int plug_num );
    virtual ~TasmotaSwitch();

    //bool SendData( unsigned char *data) override;

    std::string GetType() const override { return "TasmotaSwitch"; }
    std::string GetConfigString() const override;

    bool setRelayOn() override;
    bool setRelayOff() override;

    bool setLedOn() override;
    bool setLedOff() override;

private:
    int m_plug_num;
    CURL *m_curl;
};