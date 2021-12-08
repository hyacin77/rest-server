#ifndef __HASH_UTIL_H
#define __HASH_UTIL_H

#include <boost/uuid/detail/sha1.hpp>
#include <cstdio>
#include <string>

class HashUtil {
    public:
        static void getStringHashValueSha1(std::string &hashValue, const std::string& arg)
        {
            boost::uuids::detail::sha1 sha1;
            sha1.process_bytes(arg.data(), arg.size());
            unsigned hash[5] = {0};
            sha1.get_digest(hash);

            // Back to string
            char buf[41] = {0};

            for (int i = 0; i < 5; i++) {
                std::sprintf(buf + (i << 3), "%08x", hash[i]);
            }

            hashValue = std::string(buf);
        }
};

#endif
