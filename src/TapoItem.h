#pragma once

#include "BaseItem.h"

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
#include "log.h"

class TapoItem  : virtual public BaseItem {
public:
    TapoItem(std::string const& ip, unsigned int startChannel,
             std::string const& username = "", std::string const& password = "");
    virtual ~TapoItem();

    std::string getInfo();

protected:
    std::string sendCmd(std::string const& cmd);

    std::string m_username;
    std::string m_password;
};