
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstring>

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
#include "channeloutput/channeloutput.h"
#include "MultiSync.h"

#include "fppversion_defines.h"

#include "commands/Commands.h"

#include "TPLinkItem.h"

class TPLinkPlugin : public FPPPlugin, public httpserver::http_resource {
private:
    std::vector<std::unique_ptr <TPLinkItem>> _TPLinkOutputs;
    Json::Value config;

public:

    TPLinkPlugin() : FPPPlugin("fpp-plugin-tplink") {
        LogInfo(VB_PLUGIN, "Initializing TP-Link Plugin\n");
        readFiles();
        registerCommand();
    }
    virtual ~TPLinkPlugin() {
        _TPLinkOutputs.clear();
    }

    class SetTPLinkSwitchCommand : public Command {
    public:
        SetTPLinkSwitchCommand(TPLinkPlugin *p) : Command("Set TPLink Switch"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));
            args.push_back(CommandArg("state", "bool", "Set Switch On or Off")
                           .setDefaultValue("true"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress = "";
            bool bulbOn = true;
            if (args.size() >= 1) {
                ipAddress = args[0];
            }
            if (args.size() >= 2) {
                bulbOn = args[1]=="true";
            }
            plugin->SetSwitchState(ipAddress, bulbOn);
            return std::make_unique<Command::Result>("TPLink Switch Set");
        }
        TPLinkPlugin *plugin;
    };

    class SetTPLinkLightRGBCommand : public Command {
    public:
        SetTPLinkLightRGBCommand(TPLinkPlugin *p) : Command("Set TPLink Light RGB"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));
            args.push_back(CommandArg("r", "int", "Red").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("g", "int", "Green").setRange(0, 255).setDefaultValue("255"));
            args.push_back(CommandArg("b", "int", "Blue").setRange(0, 255).setDefaultValue("255"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress = "";
            uint8_t r = 255;
            uint8_t g = 255;
            uint8_t b = 255;
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
            plugin->SetLightOnRGB(ipAddress, r, g, b);
            return std::make_unique<Command::Result>("TPLink Light RGB Set");
        }
        TPLinkPlugin *plugin;
    };

    class SetTPLinkLightHSVCommand : public Command {
    public:
        SetTPLinkLightHSVCommand(TPLinkPlugin *p) : Command("Set TPLink Light HSV"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));
            args.push_back(CommandArg("hue", "int", "Hue").setRange(0, 360).setDefaultValue("1"));
            args.push_back(CommandArg("sat", "int", "Saturation").setRange(0, 100).setDefaultValue("100"));
            args.push_back(CommandArg("bright", "int", "Brightness").setRange(0, 100).setDefaultValue("100"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress = "";
            int hue = 1;
            int sat = 100;
            int bright = 100;
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
            plugin->SetLightOnHSV(ipAddress, hue, sat, bright);
            return std::make_unique<Command::Result>("TPLink Light HSV Set");
        }
        TPLinkPlugin *plugin;
    };
	
    class SetTPLinkLightColorTempCommand : public Command {
    public:
        SetTPLinkLightColorTempCommand(TPLinkPlugin *p) : Command("Set TPLink Switch"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));
            args.push_back(CommandArg("color_temp", "int", "Color Temp").setRange(2500, 9000).setDefaultValue("5000"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress = "";
            int colorTemp = 5000;
            if (args.size() >= 1) {
                ipAddress = args[0];
            }
            if (args.size() >= 2) {
                colorTemp = std::stoi(args[1]);
            }
            plugin->SetLightColorTemp(ipAddress, colorTemp);
            return std::make_unique<Command::Result>("TPLink Light Color Temp Set");
        }
        TPLinkPlugin *plugin;
    };

    class SetTPLinkLightOffCommand : public Command {
    public:
        SetTPLinkLightOffCommand(TPLinkPlugin *p) : Command("Set TPLink Light Off"), plugin(p) {
            args.push_back(CommandArg("IP", "string", "IP Address"));
 
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            std::string ipAddress = "";
            if (args.size() >= 1) {
                ipAddress = args[0];
            }

            plugin->SetLightOff(ipAddress);
            return std::make_unique<Command::Result>("TPLink Light Off Set");
        }
        TPLinkPlugin *plugin;
    };

    void registerCommand() {
        CommandManager::INSTANCE.addCommand(new SetTPLinkSwitchCommand(this));
        CommandManager::INSTANCE.addCommand(new SetTPLinkLightRGBCommand(this));
        CommandManager::INSTANCE.addCommand(new SetTPLinkLightHSVCommand(this));
        CommandManager::INSTANCE.addCommand(new SetTPLinkLightColorTempCommand(this));
        CommandManager::INSTANCE.addCommand(new SetTPLinkLightOffCommand(this));
    }

    virtual const std::shared_ptr<httpserver::http_response> render_GET(const httpserver::http_request &req) override {
        std::string v = getTopics();
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(v, 200));
    }
    
#if FPP_MAJOR_VERSION < 4 || FPP_MINOR_VERSION < 1
    virtual void modifyChannelData(int ms, uint8_t *seqData) override {
#else
    virtual void modifySequenceData(int ms, uint8_t *seqData) override {
#endif
        try
        {
            sendChannelData(seqData);
        }
        catch(std::exception ex)
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
        for(auto & output: _TPLinkOutputs)
        {
            output->EnableOutput();
        }
    }
    

    void sendChannelData(unsigned char *data) {
        for(auto & output: _TPLinkOutputs)
        {
            output->SendData(data);
        }
    }
    
    void saveDataToFile()
    {
        std::ofstream outfile;
        outfile.open ("/home/fpp/media/config/fpp-plugin-tplink");

        if(_TPLinkOutputs.size() ==0) {
            outfile <<  "nooutputsfound;1";
            outfile <<  "\n";
        }

        for(auto & out: _TPLinkOutputs) {
            outfile << out->GetIPAddress();
            outfile <<  ";";
            outfile << out->GetStartChannel();
            outfile <<  "\n";
        }
        outfile.close();
    }


    void readFiles()
    {
        //read topic, payload and start channel settings from JSON setting file. 
        if (LoadJsonFromFile("/home/fpp/media/config/plugin.tplink.json", config)) {
            for (int i = 0; i < config.size(); i++) {
                std::string const ip = config[i]["ip"].asString();

                unsigned int sc =  config[i]["startchannel"].asInt();
                if(!ip.empty()) {
                    LogInfo(VB_PLUGIN, "Adding IP %s SC %d\n", ip.c_str(), sc);
                    std::unique_ptr<TPLinkItem> tplinkItem = std::make_unique<TPLinkItem>(ip, sc);
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

    void SetSwitchState(std::string const& ip, bool state) {
        TPLinkItem tplinkItem(ip, 1);
        if(state){
            tplinkItem.setRelayOn();
        } else{
            tplinkItem.setRelayOff();
        }
    }

    void SetLightOnRGB(std::string const& ip, uint8_t r, uint8_t g, uint8_t b ) {
        TPLinkItem tplinkItem(ip, 1);
        tplinkItem.setLightOnRGB(r, g, b);
    }

    void SetLightOnHSV(std::string const& ip, int hue, int sat, int bright ) {
        TPLinkItem tplinkItem(ip, 1);
        tplinkItem.setLightOnHSV(hue, sat, bright);
    }

    void SetLightColorTemp(std::string const& ip, int color_temp ) {
        TPLinkItem tplinkItem(ip, 1);
        tplinkItem.setLightColorTemp(color_temp);
    }

    void SetLightOff(std::string const& ip) {
        TPLinkItem tplinkItem(ip, 1);
        tplinkItem.setLightOff();
    }
};


extern "C" {
    FPPPlugin *createPlugin() {
        return new TPLinkPlugin();
    }
}
