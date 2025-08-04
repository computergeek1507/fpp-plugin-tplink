#include "TapoItem.h"

#include <stdlib.h>
#include <cstdint>
#include <thread>
#include <cmath>

#include <iostream>
#include <istream>
#include <ostream>

#include "log.h"

TapoItem::TapoItem(std::string const& ip, unsigned int startChannel ):
    BaseItem(ip,startChannel)
{ 
}

TapoItem::~TapoItem() {

}

void TapoItem::Authenticate(std::string const& username, std::string const& password) const {
    // Implement authentication logic here if needed
    // For now, just log the username and password
    //LogInfo(VB_PLUGIN, "TapoItem Authenticate called with Username: %s, Password: %s", username.c_str(), password.c_str());

    std::string command = "--username " + username + " --password " + password;
    sendCmd(command);
    // Note: Ensure that the command is secure and does not expose sensitive information in logs.
}

std::string TapoItem::getInfo() {
    const std::string cmd = "{\"system\":{\"get_sysinfo\":{}}}";
    return sendCmd(cmd);
}

std::string TapoItem::sendCmd(std::string const& cmd) const
{
    ///home/fpp/media/plugins/fpp-plugin-tplink/env/bin/kasa
     std::string path = "/home/fpp/media/plugins/fpp-plugin-tplink/env/bin/kasa";
    if (access(path.c_str(), F_OK) == -1) {
        LogErr(VB_PLUGIN, "Kasa command not found at %s", path.c_str());
        return {};
    }
    try {
        // Prepare the command to send
        std::string command = path + " --host " + m_ipAddress + " " + cmd;
        LogInfo(VB_PLUGIN, "Sending command: %s", command.c_str());
        // Execute the command and capture the output
        FILE *pipe = popen(command.c_str(), "r");
        if (!pipe) {
            LogErr(VB_PLUGIN, "Failed to run command: %s", command.c_str());
            return {};
        }
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        LogInfo(VB_PLUGIN, "Command output: %s", result.c_str());
        return result;
    } catch (const std::exception &ex) {
        LogErr(VB_PLUGIN, "Exception occurred while sending command: %s", ex.what());
    } catch (...) {
        LogErr(VB_PLUGIN, "Unknown error occurred while sending command");
    }       

    //std::string command = "./python3 kasa --host " + m_ipAddress + " " + cmd;
    //system(command.c_str());
    return {};
}


