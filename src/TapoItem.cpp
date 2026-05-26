#include "TapoItem.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>
#include <array>

#include <iostream>
#include <istream>
#include <ostream>

static std::string getKasaPath() {
    return "/home/fpp/media/plugins/fpp-plugin-tplink/env/bin/kasa";
}

TapoItem::TapoItem(std::string const& ip, unsigned int startChannel,
                   std::string const& username, std::string const& password):
    BaseItem(ip,startChannel), m_username(username), m_password(password)
{
}

TapoItem::~TapoItem() {

}

std::string TapoItem::getInfo() {
    return sendCmd("state");
}

std::string TapoItem::sendCmd(std::string const& cmd) {
    try {
        std::string fullCmd = getKasaPath() + " --host " + m_ipAddress;
        if (!m_username.empty()) {
            fullCmd += " --username " + m_username;
        }
        if (!m_password.empty()) {
            fullCmd += " --password " + m_password;
        }
        fullCmd += " " + cmd + " 2>&1";
        LogInfo(VB_PLUGIN, "TapoItem: Running: %s\n", fullCmd.c_str());

        std::array<char, 4096> buffer;
        std::string result;

        FILE* pipe = popen(fullCmd.c_str(), "r");
        if (!pipe) {
            LogInfo(VB_PLUGIN, "TapoItem: Failed to run kasa command\n");
            m_unreachable = true;
            return std::string("");
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }

        int status = pclose(pipe);
        if (status != 0) {
            LogInfo(VB_PLUGIN, "TapoItem: kasa command failed with status %d: %s\n", status, result.c_str());
            m_unreachable = true;
            return std::string("");
        }

        m_unreachable = false;
        return result;
    }
    catch(std::exception const& ex) {
        LogInfo(VB_PLUGIN, "TapoItem: Error %s\n", ex.what());
        m_unreachable = true;
    }
    return std::string("");
}

