#ifndef __DEBUG_H
#define __DEBUG_H

#include <string>
#include <mutex>
#include <boost/format.hpp>
#include <time.h>
#include <stdio.h>

#include "SingletonDynamic.hpp"

/********************************************************************************/
/* naming rule                                                                  */
/* prefix of macro - ADM (abbreviation of DEBUGGING MACRO)               */
/* ---------------------------------------------------------------------------- */
#define MAX_DNAME       256             /* limit for length of user dir path    */
#define MAX_MBUFF       10000          /* limit for length of message buffer   */
#define MAX_LOG_HEADER  1000

#define LOG_IM  0x0001          /* logging general information          */
#define LOG_DM  0x0002          /* logging debugging messages           */
#define LOG_WM  0x0004          /* logging warning messages             */
#define LOG_EM  0x0008          /* logging error messages               */
#define LOG_AM  0x000f          /* logging messages of all types        */
#define LOG_RM  0x0010          /* logging running info. messages       */

#define LOG_NO  0x0000          /* no logging                           */
#define LOG_SO  0x0100          /* stdout                               */
#define LOG_SE  0x0200          /* stderr                               */
#define LOG_GO  0x0400          /* logging in general file              */
#define LOG_AO  0x0700          /* logging in all target                */

class Debug : public SingletonDynamic<Debug> {
    short        mode = LOG_NO;                     /* logging no messages          */
    std::string  logPath;                                 /* logging file dir             */
    std::string  logFile ;                                /* logging file name*/

    int     maxFileSize;
    int     maxFiles;

    std::mutex  mutex;
    public:
        /*
        Debug(short mode, int maxFileSize, int maxFiles, std::string logFile) {
        }
        */
    bool init(short mode, int maxFileSize, int maxFiles, std::string logFile) {
        if ((mode & 0xf800) || (mode & 0x00e0)) {
            return false;
        }
        this->mode              = mode;
        this->maxFileSize       = maxFileSize * 1024 * 1024;
        this->maxFiles          = maxFiles;
        this->logFile           = logFile;
        return true;
    }

    bool print(const char type, const char *file, const char *func, const short line, std::string msg);
};

#define LOG_ERROR(msg) ({ \
    auto debug = Debug::getInstance(); \
    if(debug) debug->print('E', __FILE__, __func__, __LINE__, msg); \
})

#define LOG_DEBUG(msg) ({ \
    auto debug = Debug::getInstance(); \
    if(debug) debug->print('D', __FILE__, __func__, __LINE__, msg); \
})

#define LOG_WARN(msg) ({ \
    auto debug = Debug::getInstance(); \
    if(debug) debug->print('W', __FILE__, __func__, __LINE__, msg); \
})

#define LOG_INFO(msg) ({ \
    auto debug = Debug::getInstance(); \
    if(debug) debug->print('I', __FILE__, __func__, __LINE__, msg); \
})

#endif
