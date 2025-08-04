#include "fpp-pch.h"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstring>
#include <execution>
#include <istream>
#include <ostream>
#include <iostream>
#include <thread>
#include <vector>

#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <thread>
#include <cmath>

#include <httpserver.hpp>
#include "common.h"
#include "settings.h"
#include "Plugin.h"
#include "Plugins.h"
#include "log.h"
#include "MultiSync.h"

#if __has_include("channeloutput/ChannelOutputSetup.h")
#include "channeloutput/ChannelOutputSetup.h"
#elif __has_include("channeloutput/channeloutput.h")
#include "channeloutput/channeloutput.h"
#endif

#include "fppversion_defines.h"

#include "commands/Commands.h"

#include "TPLinkLight.h"
#include "TPLinkSwitch.h"
#include "TPLinkItem.h"

#include "BaseLight.h"
#include "BaseSwitch.h"
#include "BaseItem.h"

#include "GoveeLight.h"
#include "TasmotaLight.h"
#include "TasmotaSwitch.h"

#include "TapoLight.h"
#include "TapoSwitch.h"
#include "TapoItem.h"

#define LIGHT_TYPES {"tplink", "tasmota", "govee", "tapo"}
#define SWITCH_TYPES {"tplink", "tasmota", "tapo"}

using namespace std::chrono_literals;

class TPLinkPlugin : public FPPPlugin, public httpserver::http_resource {
private:
    std::vector<std::unique_ptr <BaseItem>> _TPLinkOutputs;
    Json::Value config;

    std::string username;
    std::string password;

public:
    TPLinkPlugin() : FPPPlugin("fpp-plugin-tplink") {
        LogInfo(VB_PLUGIN, "Initializing TP-Link Plugin\n");
        readFiles();
        registerCommand();
    }
    virtual ~TPLinkPlugin() {
        _TPLinkOutputs.clear();
    }

    class TPLinkSetSwitchCommand : public Command {
    public:
        TPLinkSetSwitchCommand(TPLinkPlugin *p) : Command("TPLink Set Switch"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));            
            args.push_back(CommandArg("state", "bool", "Set Switch On or Off").setDefaultValue("true"));
            args.push_back(CommandArg("plug", "int", "Set Plug Number").setRange(0, 255).setDefaultValue("0"));
            args.push_back(CommandArg("type", "string", "Switch Type").setContentList(SWITCH_TYPES).setDefaultValue("tplink"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress;
            bool bulbOn = true;
            int plug_num = 0;
            std::string switch_type;
            if (args.size() >= 1) {
                ipAddress = args[0];
            }
            if (args.size() >= 2) {
                bulbOn = args[1]=="true";
            }
            if (args.size() >= 3) {
                plug_num = std::stoi(args[2]);
            }
            if (args.size() >= 4) {
                switch_type = args[3];
            }
            plugin->SetSwitchState(ipAddress, bulbOn, plug_num, switch_type);
            return std::make_unique<Command::Result>("TPLink Switch Set");
        }
        TPLinkPlugin *plugin;
    };

     class TPLinkToggleSwitchCommand : public Command {
    public:
        TPLinkToggleSwitchCommand(TPLinkPlugin *p) : Command("TPLink Toggle Switch"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));            
            args.push_back(CommandArg("delay", "int", "Delay MS").setRange(1, 10000).setDefaultValue("100"));
            args.push_back(CommandArg("plug", "int", "Set Plug Number").setRange(0, 255).setDefaultValue("0"));
            args.push_back(CommandArg("type", "string", "Switch Type").setContentList(SWITCH_TYPES).setDefaultValue("tplink"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress;
            std::chrono::milliseconds delay = 10ms;
            int plug_num = 0;
            std::string switch_type;
            if (args.size() >= 1) {
                ipAddress = args[0];
            }
            if (args.size() >= 2) {
                 delay = std::chrono::milliseconds(std::stoi(args[1]));
            }
            if (args.size() >= 3) {
                plug_num = std::stoi(args[2]);
            }
            if (args.size() >= 4) {
                switch_type = args[3];
            }
            plugin->SetSwitchState(ipAddress, false, plug_num, switch_type);
            std::this_thread::sleep_for(delay);
            plugin->SetSwitchState(ipAddress, true, plug_num, switch_type);
            return std::make_unique<Command::Result>("TPLink Switch Toggle");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkSetLightRGBCommand : public Command {
    public:
        TPLinkSetLightRGBCommand(TPLinkPlugin *p) : Command("TPLink Set Light RGB"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));
            args.push_back(CommandArg("r", "int", "Red").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("g", "int", "Green").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("b", "int", "Blue").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("color_temp", "int", "Color Temp").setRange(0, 9000).setDefaultValue("0"));
            args.push_back(CommandArg("period", "int", "Delay in ms").setRange(0, 30000).setDefaultValue("0"));
            args.push_back(CommandArg("type", "string", "Light Type").setContentList(LIGHT_TYPES).setDefaultValue("tplink"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress;
            uint8_t r = 255;
            uint8_t g = 255;
            uint8_t b = 255;
            int colorTemp = 0;
            int period = 0;
            std::string light_type;
            if (args.size() >= 1) {
                ipAddress = args[0];
            }
            if (args.size() >= 2) {
                r = std::stoi(args[1]);
            }
            if (args.size() >= 3) {
                g = std::stoi(args[2]);
            }
            if (args.size() >= 4) {
                b = std::stoi(args[3]);
            }
            if (args.size() >= 5) {
                colorTemp = std::stoi(args[4]);
            }
            if (args.size() >= 6) {
                period = std::stoi(args[5]);
            }
            if (args.size() >= 7) {
                light_type = args[6];
            }
            plugin->SetLightOnRGB(ipAddress, r, g, b, colorTemp, period, light_type);
            return std::make_unique<Command::Result>("TPLink Light RGB Set");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkSetLightHSVCommand : public Command {
    public:
        TPLinkSetLightHSVCommand(TPLinkPlugin *p) : Command("TPLink Set Light HSV"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));
            args.push_back(CommandArg("hue", "int", "Hue").setRange(0, 360).setDefaultValue("1"));
            args.push_back(CommandArg("sat", "int", "Saturation").setRange(0, 100).setDefaultValue("100"));
            args.push_back(CommandArg("bright", "int", "Brightness").setRange(0, 100).setDefaultValue("100"));
            args.push_back(CommandArg("color_temp", "int", "Color Temp").setRange(0, 9000).setDefaultValue("0"));
            args.push_back(CommandArg("period", "int", "Delay in ms").setRange(0, 30000).setDefaultValue("0"));
            args.push_back(CommandArg("type", "string", "Light Type").setContentList(LIGHT_TYPES).setDefaultValue("tplink"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress;
            int hue = 1;
            int sat = 100;
            int bright = 100;
            int colorTemp = 0;
            int period = 0;
            std::string light_type;
            if (args.size() >= 1) {
                ipAddress = args[0];
            }
            if (args.size() >= 2) {
                hue = std::stoi(args[1]);
            }
            if (args.size() >= 3) {
                sat = std::stoi(args[2]);
            }
            if (args.size() >= 4) {
                bright = std::stoi(args[3]);
            }
            if (args.size() >= 5) {
                colorTemp = std::stoi(args[4]);
            }
            if (args.size() >= 6) {
                period = std::stoi(args[5]);
            }
            if (args.size() >= 7) {
                light_type = args[6];
            }
            plugin->SetLightOnHSV(ipAddress, hue, sat, bright, colorTemp, period, light_type);
            return std::make_unique<Command::Result>("TPLink Light HSV Set");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkSetLightOffCommand : public Command {
    public:
        TPLinkSetLightOffCommand(TPLinkPlugin *p) : Command("TPLink Set Light Off"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address")); 
            args.push_back(CommandArg("type", "string", "Light Type").setContentList(LIGHT_TYPES).setDefaultValue("tplink"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress;
            std::string light_type;
            if (args.size() >= 1) {
                ipAddress = args[0];
            }
            if (args.size() >= 2) {
                light_type = args[1];
            }
            plugin->SetLightOff(ipAddress, light_type);
            return std::make_unique<Command::Result>("TPLink Light Off Set");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkAllSwitchesOffCommand : public Command {
    public:
        TPLinkAllSwitchesOffCommand(TPLinkPlugin *p) : Command("TPLink All Switches Off"), plugin(p) {
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {

            plugin->turnSwitchesOff();
            return std::make_unique<Command::Result>("TPLink All Switches Off");
        }
        TPLinkPlugin *plugin;
    };
    
    class TPLinkAllSwitchesOnCommand : public Command {
    public:
        TPLinkAllSwitchesOnCommand(TPLinkPlugin *p) : Command("TPLink All Switches On"), plugin(p) {
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {

            plugin->turnSwitchesOn();
            return std::make_unique<Command::Result>("TPLink All Switches On");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkAllLightsRGBCommand : public Command {
    public:
        TPLinkAllLightsRGBCommand(TPLinkPlugin *p) : Command("TPLink All Lights RGB"), plugin(p) {
            args.push_back(CommandArg("r", "int", "Red").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("g", "int", "Green").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("b", "int", "Blue").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("color_temp", "int", "Color Temp").setRange(0, 9000).setDefaultValue("0"));
            args.push_back(CommandArg("period", "int", "Delay in ms").setRange(0, 30000).setDefaultValue("0"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            uint8_t r = 255;
            uint8_t g = 255;
            uint8_t b = 255;
            int colorTemp = 0;
            int period = 0;
            if (args.size() >= 1) {
                r = std::stoi(args[0]);
            }
            if (args.size() >= 2) {
                g = std::stoi(args[1]);
            }
            if (args.size() >= 3) {
                b = std::stoi(args[2]);
            }
            if (args.size() >= 4) {
                colorTemp = std::stoi(args[3]);
            }
            if (args.size() >= 5) {
                period = std::stoi(args[4]);
            }
            plugin->turnLightsRGB( r, g, b, colorTemp, period);
            return std::make_unique<Command::Result>("TPLink All Lights RGB Set");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkAllLightsHSVCommand : public Command {
    public:
        TPLinkAllLightsHSVCommand(TPLinkPlugin *p) : Command("TPLink All Lights HSV"), plugin(p) {
            args.push_back(CommandArg("hue", "int", "Hue").setRange(0, 360).setDefaultValue("1"));
            args.push_back(CommandArg("sat", "int", "Saturation").setRange(0, 100).setDefaultValue("100"));
            args.push_back(CommandArg("bright", "int", "Brightness").setRange(0, 100).setDefaultValue("100"));
            args.push_back(CommandArg("color_temp", "int", "Color Temp").setRange(0, 9000).setDefaultValue("0"));
            args.push_back(CommandArg("period", "int", "Delay in ms").setRange(0, 30000).setDefaultValue("0"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            int hue = 1;
            int sat = 100;
            int bright = 100;
            int colorTemp = 0;
            int period = 0;
            if (args.size() >= 1) {
                hue = std::stoi(args[0]);
            }
            if (args.size() >= 2) {
                sat = std::stoi(args[1]);
            }
            if (args.size() >= 3) {
                bright = std::stoi(args[2]);
            }
            if (args.size() >= 4) {
                colorTemp = std::stoi(args[3]);
            }
            if (args.size() >= 5) {
                period = std::stoi(args[4]);
            }
            plugin->turnLightsHSV( hue, sat, bright, colorTemp, period);
            return std::make_unique<Command::Result>("TPLink All Lights HSV Set");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkAllLightsOffCommand : public Command {
    public:
        TPLinkAllLightsOffCommand(TPLinkPlugin *p) : Command("TPLink All Lights Off"), plugin(p) {
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {

            plugin->turnLightsOff();
            return std::make_unique<Command::Result>("TPLink All Lights Off");
        }
        TPLinkPlugin *plugin;
    };

    class TPLinkAllSwitchesToggleCommand : public Command {
    public:
        TPLinkAllSwitchesToggleCommand(TPLinkPlugin *p) : Command("TPLink All Switches Toggle"), plugin(p) {
            args.push_back(CommandArg("delay", "int", "Delay MS").setRange(1, 10000).setDefaultValue("100"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::chrono::milliseconds delay = 10ms;
            if (args.size() >= 1) {
                delay = std::chrono::milliseconds(std::stoi(args[0]));
            }
            plugin->turnSwitchesOff();
            std::this_thread::sleep_for(delay);
            plugin->turnSwitchesOn();
            return std::make_unique<Command::Result>("TPLink All Switches Toggle");
        }
        TPLinkPlugin *plugin;
    };

    void registerCommand() {
        CommandManager::INSTANCE.addCommand(new TPLinkSetSwitchCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkToggleSwitchCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkSetLightRGBCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkSetLightHSVCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkSetLightOffCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkAllSwitchesOnCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkAllSwitchesOffCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkAllLightsRGBCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkAllLightsHSVCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkAllLightsOffCommand(this));
        CommandManager::INSTANCE.addCommand(new TPLinkAllSwitchesToggleCommand(this));
    }

    virtual HTTP_RESPONSE_CONST std::shared_ptr<httpserver::http_response> render_GET(const httpserver::http_request &req) override {
        std::string v = getTopics();
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(v, 200));
    }
    
    virtual void modifySequenceData(int ms, uint8_t *seqData) override {
        try
        {
            sendChannelData(seqData);
        }
        catch(std::exception const& ex)
        {
            std::cout << ex.what();
        }
    }

    virtual void playlistCallback(const Json::Value &playlist, const std::string &action, const std::string &section, int item) {
        if (settings["Start"] == "PlaylistStart" && action == "start") {
            EnableTPLinkItems();
        }  
    }

    void EnableTPLinkItems() {
        for(auto & output: _TPLinkOutputs) {
            output->EnableOutput();
        }
    }    

    void sendChannelData(unsigned char *data) {
        //for(auto & output: _TPLinkOutputs) {
        //    output->SendData(data);
        //}
        std::for_each(std::execution::par, std::begin(_TPLinkOutputs), std::end(_TPLinkOutputs), [data](auto& output) {
            output->SendData(data);
        });
    }

    void turnSwitchesOff() {
        std::for_each(std::execution::par, std::begin(_TPLinkOutputs), std::end(_TPLinkOutputs), [](auto& output) {
            auto* derived = dynamic_cast<BaseSwitch*>(output.get());
            if (derived) {
                derived->EnableOutput();
                derived->setRelayOff();
            }
        });
    }

    void turnSwitchesOn() {
        std::for_each(std::execution::par, std::begin(_TPLinkOutputs), std::end(_TPLinkOutputs), [](auto& output) {
            auto* derived = dynamic_cast<BaseSwitch*>(output.get());
            if (derived) {
                derived->EnableOutput();
                derived->setRelayOn();
            }
        });
    }

    void turnLightsRGB(uint8_t r, uint8_t g, uint8_t b, int color_temp, int period) {
        std::for_each(std::execution::par, std::begin(_TPLinkOutputs), std::end(_TPLinkOutputs),
         [r, g, b, color_temp, period](auto& output) {
            auto* derived = dynamic_cast<BaseLight*>(output.get());
            if (derived) {
                derived->EnableOutput();
                derived->setLightOnRGB(r, g, b, color_temp, period);
            }
        });
    }

    void turnLightsHSV(int hue, int sat, int bright, int color_temp, int period) {
        std::for_each(std::execution::par, std::begin(_TPLinkOutputs), std::end(_TPLinkOutputs),
         [hue, sat, bright, color_temp, period](auto& output) {
            auto* derived = dynamic_cast<BaseLight*>(output.get());
            if (derived) {
                derived->EnableOutput();
                derived->setLightOnHSV(hue, sat, bright, color_temp, period);
            }
        });
    }

    void turnLightsOff() {
        std::for_each(std::execution::par, std::begin(_TPLinkOutputs), std::end(_TPLinkOutputs), [](auto& output) {
            auto* derived = dynamic_cast<BaseLight*>(output.get());
            if (derived) {
                derived->EnableOutput();
                derived->setLightOff();
            }
        });
    }
    
    void saveDataToFile()
    {
        std::ofstream outfile;
#if FPP_MAJOR_VERSION < 6
        outfile.open ("/home/fpp/media/config/fpp-plugin-tplink");
#else
        outfile.open (FPP_DIR_CONFIG("/fpp-plugin-tplink"));
#endif

        if(_TPLinkOutputs.size() ==0) {
            outfile <<  "nooutputsfound;1;null";
            outfile <<  "\n";
        }

        for(auto & out: _TPLinkOutputs) {
            outfile << out->GetIPAddress();
            outfile <<  ";";
            outfile << out->GetStartChannel();
            outfile <<  ";";
            outfile << out->GetType();
            outfile <<  "\n";
        }
        outfile.close();
    }

    std::unique_ptr<BaseItem> getLightDevicePtr(std::string const& devicetype, std::string const& ip, unsigned int sc)
    {
        std::unique_ptr<BaseItem> tplinkItem;
        if (devicetype.find("light") != std::string::npos ||
            devicetype.find("tplinklight") != std::string::npos ||
            devicetype.find("tplink") != std::string::npos) {
            tplinkItem = std::make_unique<TPLinkLight>(ip, sc);
        }  else if (devicetype.find("goveelight") != std::string::npos ||
                    devicetype.find("govee") != std::string::npos) {
            tplinkItem = std::make_unique<GoveeLight>(ip, sc);
        } else if (devicetype.find("tasmotalight") != std::string::npos ||
                   devicetype.find("tasmota") != std::string::npos) {
            tplinkItem = std::make_unique<TasmotaLight>(ip, sc);
        } else {
            LogInfo(VB_PLUGIN, "Devicetype not found '%s'", devicetype.c_str());
            tplinkItem = std::make_unique<TPLinkLight>(ip, sc);
        }
        return tplinkItem;
    }

    std::unique_ptr<BaseItem> getSwitchDevicePtr(std::string const& devicetype, std::string const& ip, unsigned int sc, int plug_num)
    {
        std::unique_ptr<BaseItem> tplinkItem;
        if (devicetype.find("switch") != std::string::npos||
            devicetype.find("tplinkswitch") != std::string::npos ||
            devicetype.find("tplink") != std::string::npos) {
            tplinkItem = std::make_unique<TPLinkSwitch>(ip, sc, plug_num);
        } else if (devicetype.find("tasmotaswitch") != std::string::npos ||
                    devicetype.find("tasmota") != std::string::npos) {
            tplinkItem = std::make_unique<TasmotaSwitch>(ip, sc, plug_num);
        } else {
            LogInfo(VB_PLUGIN, "Devicetype not found '%s'", devicetype.c_str());
            tplinkItem = std::make_unique<TPLinkSwitch>(ip, sc, plug_num);
        }
        return tplinkItem;
    }

    void readFiles()
    {
        //read topic, payload and start channel settings from JSON setting file. 
#if FPP_MAJOR_VERSION < 6
        std::string configLocation = ("/home/fpp/media/config/plugin.tplink.json");
#else
        std::string configLocation = FPP_DIR_CONFIG("/plugin.tplink.json");
#endif
        if (LoadJsonFromFile(configLocation, config)) {
            username = config.get("username", "").asString();
            password = config.get("password", "").asString();
            //LogInfo(VB_PLUGIN, "TPLink Plugin Username: %s", username.c_str());
            //LogInfo(VB_PLUGIN, "TPLink Plugin Password: %s", password.c_str());

            for (unsigned int i = 0; i < config.size(); i++) {
                std::string const ip = config[i]["ip"].asString();
                std::string const devicetype = config[i].get("devicetype","light").asString();
                unsigned int sc =  config[i].get("startchannel",1).asInt();
                if(!ip.empty()) {
                    std::unique_ptr<BaseItem> tplinkItem;
                    if (devicetype.find("goveelight") != std::string::npos) {
                        tplinkItem = std::make_unique<GoveeLight>(ip, sc);
                    } else if (devicetype.find("tasmotalight") != std::string::npos) {
                        tplinkItem = std::make_unique<TasmotaLight>(ip, sc);
                    } else if (devicetype.find("tasmotaswitch") != std::string::npos) {
                        int const plugNum =  config[i].get("plugnumber", 0).asInt();
                        tplinkItem = std::make_unique<TasmotaSwitch>(ip, sc, plugNum);
                    } else if (devicetype.find("tapolight") != std::string::npos) {
                        tplinkItem = std::make_unique<TapoLight>(ip, sc);
                        tplinkItem->Authenticate(username, password);
                    } else if (devicetype.find("taposwitch") != std::string::npos) {
                        int const plugNum =  config[i].get("plugnumber", 0).asInt();
                        tplinkItem = std::make_unique<TapoSwitch>(ip, sc, plugNum);
                        tplinkItem->Authenticate(username, password);
                    } else if (devicetype.find("light") != std::string::npos) {
                        tplinkItem = std::make_unique<TPLinkLight>(ip, sc);
                    } else if (devicetype.find("switch") != std::string::npos) {
                        int const plugNum =  config[i].get("plugnumber", 0).asInt();
                        tplinkItem = std::make_unique<TPLinkSwitch>(ip, sc, plugNum);
                    } else {
                        LogInfo(VB_PLUGIN, "Devicetype not found '%s'", devicetype.c_str());
                        tplinkItem = std::make_unique<TPLinkLight>(ip, sc);
                    }
                    LogInfo(VB_PLUGIN, "Added %s\n", tplinkItem->GetConfigString().c_str());
                    _TPLinkOutputs.push_back(std::move(tplinkItem));
                }
            }
        }
        saveDataToFile();
    }
    
    std::string getTopics()
    {
        std::string topics;
        for(auto & out: _TPLinkOutputs) {
            topics += out->GetIPAddress();
            topics += ",";
        }
        return topics;
    } 

    void SetSwitchState(std::string const& ip, bool state, int plug_num, std::string const& type) {

        auto SetSwState = [this,state,plug_num,type](std::string const& __ip)
        {
            auto sswitch = getSwitchDevicePtr(type, __ip, 1, plug_num);
            auto* derived = dynamic_cast<BaseSwitch*>(sswitch.get());
            if (derived) {
                if(state){
                    derived->setRelayOn();
                } else{
                    derived->setRelayOff();
                }
            }
        };
        if(ip.find(",") != std::string::npos) {
            auto ips = split(ip, ',');
            for(auto const& ip_ : ips) {
               SetSwState(ip_);
            }
        } else {
            SetSwState(ip);
        }
    }

    void SetLightOnRGB(std::string const& ip, uint8_t r, uint8_t g, uint8_t b, int color_temp, int period , std::string const& type) {
        auto SetLightState = [this, r, g, b, color_temp, period, type](std::string const& __ip)
        {
            auto light = getLightDevicePtr(type, __ip, 1);
            auto* derived = dynamic_cast<BaseLight*>(light.get());
            if (derived) {
                derived->setLightOnRGB(r, g, b, color_temp, period);
            }
        };
        if(ip.find(",") != std::string::npos) {
            auto ips = split(ip, ',');
            for(auto const& ip_ : ips) {
               SetLightState(ip_);
            }
        } else {
            SetLightState(ip);
        }
    }

    void SetLightOnHSV(std::string const& ip, int hue, int sat, int bright, int color_temp, int period, std::string const& type) {
        auto SetLightState = [this, hue, sat, bright, color_temp, period, type](std::string const& __ip)
        {
            auto light = getLightDevicePtr(type, __ip, 1);
            auto* derived = dynamic_cast<BaseLight*>(light.get());
            if (derived) {
                derived->setLightOnHSV(hue, sat, bright, color_temp, period);
            }
            //BaseLight tplinkLight(__ip, 1);
            //tplinkLight.setLightOnHSV(hue, sat, bright, color_temp, period);
        };
        if(ip.find(",") != std::string::npos) {
            auto ips = split(ip, ',');
            for(auto const& ip_ : ips) {
               SetLightState(ip_);
            }
        } else {
            SetLightState(ip);
        }
    }

    void SetLightOff(std::string const& ip, std::string const& type) {
        auto SetLightState = [this, type](std::string const& __ip)
        {
            auto light = getLightDevicePtr(type, __ip, 1);
            auto* derived = dynamic_cast<BaseLight*>(light.get());
            if (derived) {
                derived->setLightOff();
            }
            //BaseLight tplinkLight(__ip, 1);
            //tplinkLight.setLightOff();
        };
        if(ip.find(",") != std::string::npos) {
            auto ips = split(ip, ',');
            for(auto const& ip_ : ips) {
               SetLightState(ip_);
            }
        } else {
            SetLightState(ip);
        }
    }
};


extern "C" {
    FPPPlugin *createPlugin() {
        return new TPLinkPlugin();
    }
}
