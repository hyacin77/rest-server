#ifndef __BLOCK_MANAGER_H
#define __BLOCK_MANAGER_H

#include <unordered_map>

#include "Singleton.hpp"
#include "BlockInfo.hpp"
#include "Interface.hpp"

class BlockManager: public Singleton<BlockManager> {
    struct hash_pair {
        template <class T1, class T2>
            size_t operator()(const std::pair<T1, T2>& p) const
            {
                auto hash1 = std::hash<T1>{}(p.first);
                auto hash2 = std::hash<T2>{}(p.second);
                return hash1 ^ hash2;
            }
    };

    std::unordered_map<std::pair<std::string, std::string>, BlockInfo *, hash_pair>	managedBlocks;
    Interface   interface;

    public:
    std::string single;
    bool init();

    bool addBlock(BlockInfo *bi) {
        this->managedBlocks.insert(std::make_pair(bi->getKey(), bi));
        return true;
    };

    bool delBlock(std::string name, std::string version) {
        auto it = this->managedBlocks.find(std::make_pair(name, version));
        delete it->second;
        this->managedBlocks.erase(it);
        return true;
    }

    BlockInfo * getBlock(std::string name, std::string version) {
        auto it = this->managedBlocks.find(std::make_pair(name, version));
        if(it != this->managedBlocks.end()) {
            return it->second;
        }
        return nullptr;
    }
};

#endif
