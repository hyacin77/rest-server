#include <iostream>
#include <fstream>

#include "Debug.hpp"

using std::cout;
using std::endl;

#define CHECK_FILE_SIZE(logout, size) ({ \
    logout.seekp(0, std::ios::end); \
    int length_logout = logout.tellp(); \
    logout.seekp(0, std::ios::beg); \
    length_logout > size ? true : false; \
})

#define LOG_FILE_ROTATE(writeFile, logout, maxFiles) ({\
        logout << "Logging file changed!!" << endl; \
        logout.close(); \
        for(int i = maxFiles; i > 0; i--) { \
            char to[MAX_DNAME] = {0,}; \
            char from[MAX_DNAME] = {0,}; \
            snprintf(to, MAX_DNAME, "%s.%02d", writeFile.c_str(), i); \
            if(access(to, F_OK) == 0) { \
                remove(to); \
            } \
            if(i - 1 == 0) snprintf(from, MAX_DNAME, "%s", writeFile.c_str()); \
            else snprintf(from, MAX_DNAME, "%s.%02d", writeFile.c_str(), i - 1); \
            if(access(from, F_OK) == 0) { \
                rename(from, to); \
            } \
        } \
        logout.open(writeFile, std::ios::app); \
})

#define CHECK_LOG_TYPE(mode, type) ({\
    bool log_type_ret = true; \
    switch(type) { \
        case 'I': \
            if (!(mode & LOG_IM)) log_type_ret = false; \
                break; \
        case 'D': \
            if (!(mode & LOG_DM)) log_type_ret = false; \
                break; \
        case 'W': \
            if (!(mode & LOG_WM)) log_type_ret = false; \
                break; \
        case 'E': \
            if (!(mode & LOG_EM)) log_type_ret = false; \
                break; \
    } \
    log_type_ret; \
})

bool Debug::print(const char type, const char *file, const char *func, const short line, std::string msg) {
    auto temp = time(nullptr);
    if(temp == -1) return false;
    struct tm curtime;
    if(localtime_r(&temp, &curtime) == nullptr) return false;
    char head[MAX_LOG_HEADER] = {0,};
    snprintf(head, MAX_LOG_HEADER, "[%c][%04d%02d%02d:%02d%02d%02d][%8x:"
            "%-11.11s:%-11.11s:%05d] ", type, curtime.tm_year+1900, curtime.tm_mon + 1, curtime.tm_mday, curtime.tm_hour, curtime.tm_min,
            curtime.tm_sec, (u_int)pthread_self(), file, func, line);
    
    //LOCK
    std::string writeFile = this->logFile + ".log";
    this->mutex.lock();
    std::ofstream logout;
    //std::cout << writeFile << std::endl;
    logout.open(writeFile, std::ios::app);

    if(CHECK_FILE_SIZE(logout, this->maxFileSize)) { // file size bigger than max
        LOG_FILE_ROTATE(writeFile, logout, this->maxFiles);
    } 

    if(!CHECK_LOG_TYPE(this->mode, type)) {
        goto out;
    }

    if(this->mode & LOG_SO) std::cout << std::string(head) + msg << std::endl;

    logout << std::string(head) << msg << std::endl;

out:
    logout.close();
    //UNLOCK
    this->mutex.unlock();

    return true;
}
