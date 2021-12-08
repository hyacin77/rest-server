#ifndef __GLOBAL_CONFIG_H
#define __GLOBAL_CONFIG_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "SingletonDynamic.hpp"
#include "Debug.hpp"

using std::cout;
using std::cerr;
using std::endl;

namespace pt = boost::property_tree;

class GlobalConfig : public SingletonDynamic<GlobalConfig> {
    std::string home;
    std::string configFile;
    pt::ptree   root;
    std::mutex  mutex;

    public:
    int getIntegerValue(std::string param) {
        this->mutex.lock();
        int value = root.get<int>(param);
        this->mutex.unlock();
        return value;
    }

    std::string getStringValue(std::string param) {
        this->mutex.lock();
        std::string value = root.get<string>(param);
        this->mutex.unlock();
        return value;
    }

    pt::ptree& getTreeValue(std::string param) {
        this->mutex.lock();
        pt::ptree& value = root.get_child(param);
        this->mutex.unlock();
        return value;
    }

    bool init(std::string home, std::string configFile) {
        this->home = home;
        if(this->home.size() < 1) return false;
        this->configFile = this->home + "/cfg/" + configFile;
        try {
            pt::read_json(this->configFile, this->root);
        } catch (std::exception &e) {
            cerr << "fail to load configuation from file[" << this->configFile << "], err:" << e.what() << endl;
            return false;
        }

        return true;
    }
};


#endif
