#pragma once

#include <stdlib.h>
#include <list>
#include <string>
#include <atomic>

//#include "common.h"
//#include "log.h"

class BaseItem {
public:
    BaseItem(std::string const& ip, unsigned int startChannel );
    virtual ~BaseItem();

    std::string GetIPAddress() const { return m_ipAddress; }
    unsigned int GetStartChannel() const { return m_startChannel; }

    virtual void EnableOutput() { m_unreachable = false; m_issending = false; }

    virtual bool SendData(unsigned char *data) = 0;

    virtual std::string GetType() const = 0;// {return "";};
    virtual std::string GetConfigString() const;

    virtual void Authenticate( std::string const& username, std::string const& password) const {  }

protected:
    std::string m_ipAddress;
    
    unsigned int m_startChannel;
    unsigned int m_seqCount;

    std::atomic<bool> m_unreachable;
    std::atomic<bool> m_issending;

};