#ifndef __COMMON_UTIL_H
#define __COMMON_UTIL_H

#include <vector>
#include <string>
#include <boost/beast/core/detail/base64.hpp>
#include <arpa/inet.h>
#include <boost/date_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace boost::posix_time;
using namespace boost::local_time;

#ifdef WORDS_BIGENDIAN
#       define ntohll(n)        (n)
#       define htonll(n)        (n)
#else
#       define ntohll(n)        (((uint64_t)ntohl(n)) << 32) + ntohl((n) >> 32)
#       define htonll(n)        (((uint64_t)htonl(n)) << 32) + htonl((n) >> 32)
#endif


class CommonUtil {
    public:
    static std::vector<std::vector<std::string>> getParameterFromUri(std::string params) {
        int start = 0;
        int end = 0;
        std::vector<std::vector<std::string>> paramVec;
        while(true) {
            end = params.find('=', start);
            std::string param = params.substr(start, end - start);
            start = end;
            end = params.find('&', start);
            std::string value = end == std::string::npos ? params.substr(start + 1) : params.substr(start + 1, end - start - 1);
            paramVec.push_back({param, value});
            if(end == std::string::npos) break;
            start = end + 1;
        }
        return paramVec;
    }

    static std::string getParameterValue(std::vector<std::vector<std::string>>& paramVec, std::string param) {
        for(int i = 0; i < paramVec.size(); i++) {
            if(paramVec[i][0] == param) {
                return paramVec[i][1];
            }
        }
        return "";
    }

    static std::string getBase64Encoded(std::string& data) {
        std::string encoded;
        encoded.resize(data.size() * 2);
        boost::beast::detail::base64::encode((void *)encoded.data(), data.c_str(), data.size());
        int size = strlen(encoded.c_str());
        return encoded.substr(0, size);
    }

    static std::string getBase64Decoded(std::string& data) {
        std::string decoded;
        decoded.resize(data.size() * 2);
        auto ret = boost::beast::detail::base64::decode((void *)decoded.data(), data.c_str(), data.size());
        //int size = strlen(decoded.c_str());
        //std::cout << "first[" << ret.first << "] second[" << ret.second << "] size[" <<  size << "]" << std::endl;
        return decoded.substr(0, ret.first);
    }

    static std::string getCurrentTimeRfc3399() {
        local_date_time t = local_sec_clock::local_time(time_zone_ptr());
        local_time_facet* lf(new local_time_facet("%Y-%m-%dT%H:%M:%S%F%Q"));
        std::stringstream rfc3399;
        rfc3399.imbue(std::locale(std::cout.getloc(), lf));
        rfc3399 << t;
        return rfc3399.str();
    }

    static void getXoredData(string& dst, string src, string& otpKey) {
        std::string temp;
        temp.resize(src.size());
        for(int i = 0; i < src.size(); i++) {
            unsigned char a = src[i];
            unsigned char b = otpKey[i];
            temp[i] = a ^ b;
            //printf("[%d] tmp[%02X] src[%02X] key[%02X]\n", i, (unsigned char)temp[i], a, b);
        }
        dst = temp;
    }

    static bool getRandom(std::shared_ptr<string> data, int size) {
        std::ifstream file;
        data->resize(size);
        file.open("/dev/random");
        if(file.is_open()) {
            file.read((char *)data->data(), size);
        }
        file.close();
    }
};














#endif
