#pragma once

#include "KasaKLAPItem.h"
#include "BaseLight.h"

class KasaKLAPLight : public KasaKLAPItem, public BaseLight {
public:
    KasaKLAPLight(std::string const& ip, unsigned int startChannel,
              tplink::tapo::Credentials const& credentials);
    virtual ~KasaKLAPLight();

    std::string GetType() const override { return "KasaKLAPLight"; }

    bool setLightOnRGB(uint8_t r, uint8_t g, uint8_t b, int color_Temp = 0, int period = 0) override;
    bool setLightOnHSV(int hue, int saturation, int brightness, int color_Temp = 0, int period = 0) override;
    bool setLightOff() override;
};
