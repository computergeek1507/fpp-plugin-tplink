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
    TapoItem(std::string const& ip, unsigned int startChannel );
    virtual ~TapoItem();

    std::string getInfo();

    void Authenticate( std::string const& username, std::string const& password) const override;
protected:
    std::string sendCmd(std::string const& cmd) const;

private:

};