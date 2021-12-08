#ifndef __UUID_UTIL_H
#define __UUID_UTIL_H

#include <iostream>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

using boost::uuids::uuid;
using std::string;

class UuidUtil {
    public:
    static uuid getRandomUuid() {
        boost::uuids::random_generator  randomGenerator;
        return randomGenerator();
    }
    static uuid getNameUuid(string name) {
        boost::uuids::name_generator    nameGenerator({00000000-0000-0000-0000-000000000000});
        return nameGenerator(name);
    }
    static uuid getUuidString(string uuid) {
        boost::uuids::string_generator  stringGenerator;
        return stringGenerator(uuid);
    }
    static string getString(uuid u) {
        return boost::uuids::to_string(u);
    }
};

#endif
