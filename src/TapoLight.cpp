#include "TapoLight.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>
#include <string>

#include <iostream>

TapoLight::TapoLight(std::string const& ip, unsigned int startChannel,
                     std::string const& username, std::string const& password) :
    BaseItem(ip, startChannel), TapoItem(ip, startChannel, username, password),
    BaseLight(ip, startChannel)
{
}

TapoLight::~TapoLight() {
}

bool TapoLight::setLightOnHSV(int hue, int saturation, int brightness, int color_Temp, int period) {
    std::string cmd = "hsv " + std::to_string(hue) + " " +
                      std::to_string(saturation) + " " +
                      std::to_string(brightness);
    return !sendCmd(cmd).empty();
}

bool TapoLight::setLightOff() {
    return !sendCmd("off").empty();
}

bool TapoLight::setLightOnRGB(uint8_t r, uint8_t g, uint8_t b, int color_Temp, int period) {
    float h, si, sv, i, v;
    RGBtoHSIV(r / 255.0f, g / 255.0f, b / 255.0f, h, si, sv, i, v);

    int ih = static_cast<int>(h);
    int isv = static_cast<int>(sv * 100);
    int iv = static_cast<int>(v * 100);
    return setLightOnHSV(ih, isv, iv, color_Temp, period);
}
