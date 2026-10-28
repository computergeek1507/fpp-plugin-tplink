#include "TapoLight.h"

#include <algorithm>
#include <string>

TapoLight::TapoLight(std::string const& ip, unsigned int startChannel,
                     std::string const& username, std::string const& password) :
    BaseItem(ip, startChannel), TapoItem(ip, startChannel, username, password),
    BaseLight(ip, startChannel)
{
}

TapoLight::~TapoLight() {
}

// Tapo bulbs take hue 0-360, saturation 0-100 and brightness 1-100; a colour
// temperature, when given (in kelvin), takes precedence over hue and saturation.
// Brightness 0 turns the bulb off. Tapo has no fade time, so `period` is unused.
bool TapoLight::setLightOnHSV(int hue, int saturation, int brightness, int color_Temp, int period) {
    if (brightness <= 0) {
        return setLightOff();
    }
    hue = ((hue % 360) + 360) % 360;
    saturation = std::max(0, std::min(100, saturation));
    brightness = std::max(1, std::min(100, brightness));

    std::string params = "{\"device_on\":true,\"brightness\":" + std::to_string(brightness);
    if (color_Temp > 0) {
        params += ",\"color_temp\":" + std::to_string(color_Temp);
    } else {
        params += ",\"hue\":" + std::to_string(hue) + ",\"saturation\":" + std::to_string(saturation) +
                  ",\"color_temp\":0";
    }
    params += "}";
    tplink::json::Value result;
    return call("set_device_info", params, result);
}

bool TapoLight::setLightOff() {
    tplink::json::Value result;
    return call("set_device_info", "{\"device_on\":false}", result);
}

bool TapoLight::setLightOnRGB(uint8_t r, uint8_t g, uint8_t b, int color_Temp, int period) {
    float h, si, sv, i, v;
    RGBtoHSIV(r / 255.0f, g / 255.0f, b / 255.0f, h, si, sv, i, v);

    int ih = static_cast<int>(h + 0.5f);
    int isv = static_cast<int>(sv * 100 + 0.5f);
    int iv = static_cast<int>(v * 100 + 0.5f);
    return setLightOnHSV(ih, isv, iv, color_Temp, period);
}
