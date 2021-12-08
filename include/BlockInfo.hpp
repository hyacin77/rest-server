#ifndef __BLOCK_INFO_H
#define __BLOCK_INFO_H

#include <iostream>
#include <mutex>

#include "Interface.hpp"
#include "DBAccess.hpp"
#include "Debug.hpp"
#include "GlobalConfig.hpp"

class BlockInfo {
    protected:
    void            *handle;
    std::string	    name;
    std::string     version;
    std::string	    soPath; //shared object file
    int	            taskCnt;
    std::mutex      mutex;

    public:
    BlockInfo(void *handle, std::string name, std::string version, std::string soPath) {
        this->handle    = nullptr;
        this->name      = name;
        this->version   = version;
        this->soPath    = soPath;
        this->taskCnt   = 0;
    };

    ~BlockInfo() {
    };

    std::pair<std::string, std::string> getKey() {
        return std::make_pair(this->name, this->version);
    };

    void printBlock() {
        std::cout << this->name << " v:" << this->version << " file:" << this->soPath << std::endl;
    };

    virtual bool init(Interface &inf, std::shared_ptr<Debug> debug, std::shared_ptr<GlobalConfig> gConfig, std::shared_ptr<DBAccess> dba) {return true;};
};

#endif
