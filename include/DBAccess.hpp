#ifndef __DB_ACCESS_H
#define __DB_ACCESS_H

#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <mutex>
#include <thread>
#include <iostream>

#include "SingletonDynamic.hpp"
#include "Debug.hpp"

using std::vector;
using std::string;
using std::cout;
using std::cerr;
using std::endl;


#define DBError(hEnv, hDbc, hStmt, msg, nerr)                                           \
        short len;                                                                      \
        unsigned char state[10];                                                        \
        memset(msg, 0x00, sizeof(msg));                                                 \
        SQLError(hEnv, hDbc, hStmt, state, &nerr, (SQLCHAR *)msg, sizeof(msg), &len);

#define DB_Connection_Error(hEnv, hDbc, msg)                                            \
        int nerr;                                                                       \
        short len;                                                                      \
        unsigned char state[10];                                                        \
        memset(msg, 0x00, sizeof(msg));                                                 \
        SQLError(hEnv, hDbc, NULL, state, &nerr, (SQLCHAR *)msg, sizeof(msg), &len);

#define MAX_CONNECTION_TRY		60
#define MAX_ERROR_MSG           1024

enum {
    disconnected = 0,
    available,
    used
};

class Connection {
    int         state; //0: disconnected, 1: available, 2: used
    bool        isAvailable;
    SQLHDBC     hDbc;
    public:
    Connection(): state(disconnected) {}

    ~Connection() {
    }

    bool connect(SQLHENV hEnv, string dsn) {
        int     nOpt = SQL_AUTOCOMMIT_OFF;
        char    msg[MAX_ERROR_MSG] = {0,};
        char    option[256] = {0,};

        if(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &this->hDbc) != 0) {
            cerr << "Memory allocation for connection handle error!!" << endl;
            return false;
        }

        sprintf(option, "DSN=%s;", dsn.c_str());
        cout << "connect options:" << option << endl;

        if(SQLDriverConnect(this->hDbc, NULL, (SQLCHAR *)option, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE) != 0)
        {
            DB_Connection_Error(hEnv, this->hDbc, msg);
            cerr << "err:" << msg << endl;
            return false;
        }

        if(SQLSetConnectOption(this->hDbc, SQL_ATTR_AUTOCOMMIT, nOpt) == -1)
        {
            DB_Connection_Error(hEnv, this->hDbc, msg);
            cerr << "err:" << msg << endl;
            return false;
        }

        this->state = available;
        return true;
    }

    bool reconnect(SQLHENV hEnv, string dsn) {
        int     nOpt = SQL_AUTOCOMMIT_OFF;
        char    msg[MAX_ERROR_MSG] = {0,};
        char    option[256] = {0,};

        /*
         * DBC Handle 에 설정된 DB 와의 연결을 해제
         * 연결 해제 후 SQLConnect(), SQLDriverConnect() 를 수행하여
         * 다시 DB 와의 연결을 수행할 수 있음
         */
        this->state = disconnected;
        if(SQLDisconnect(this->hDbc) != 0) {
            DB_Connection_Error(hEnv, this->hDbc, msg);
            cerr << "err:" << msg << endl;
        }

        if(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &this->hDbc) != 0) {
            cerr << "Memory allocation for connection handle error!!" << endl;
            return false;
        }

        sprintf(option, "DSN=%s;", dsn.c_str());
        cout << "connect options:" << option << endl;

        if(SQLDriverConnect(this->hDbc, NULL, (SQLCHAR *)option, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE) != 0) {
            DB_Connection_Error(hEnv, this->hDbc, msg);
            cerr << "err:" << msg << endl;
            return false;
        }

        if(SQLSetConnectOption(this->hDbc, SQL_ATTR_AUTOCOMMIT, nOpt) == -1) {
            DB_Connection_Error(hEnv, this->hDbc, msg);
            cerr << "err:" << msg << endl;
            return false;
        }

        this->state = available;
        return true;
    }

    bool disconnect(SQLHENV hEnv) {
        char    msg[MAX_ERROR_MSG];
        if(SQLFreeHandle(SQL_HANDLE_DBC, this->hDbc) != 0) {
            DB_Connection_Error(hEnv, this->hDbc, msg);
            cerr << "err:" << msg << endl;
            return false;
        }

        return true;
    }

    int getState() {
        return this->state;
    }

    void setState(int state) {
        this->state = state;
    }

    SQLHDBC& getHandle() {
        return this->hDbc;
    }
};

class DBAccess: public SingletonDynamic<DBAccess> {
    string      dsn;
    SQLHENV     hEnv;
    SQLHDBC     err;
    int         usedConn;
    int         maxConn;
    std::mutex  mutex;
    vector<Connection*> pool;
    std::thread      *manager;

    public:
    bool init(string dsn, int maxConn) {
        this->dsn       = dsn;
        this->maxConn   = maxConn;
        this->usedConn  = 0;
        this->err       = nullptr;

        if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &this->hEnv) != 0) {
            cerr << "Allocation Environment Error!!" << endl;
            return false;
        }

        if(SQLSetEnvAttr(this->hEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0) != SQL_SUCCESS) {
            cerr << "ODBC Version Setting Error!!" << endl;
            return false;
        }

        for(int i = 0; i < this->maxConn; i++) {
            Connection * connection = new Connection();
            connection->connect(this->hEnv, this->dsn);
            pool.push_back(connection);
        }

        this->manager = new std::thread([this] { this->checkThread(); });
    }

    SQLHDBC& getHandle() {
        this->mutex.lock();
        for(auto connection : pool) {
            if(connection->getState() == available) {
                connection->setState(used);
                this->mutex.unlock();
                return connection->getHandle();
            }
        }
        this->mutex.unlock();
        return err;
    }

    SQLHDBC& getHandleTry() {
        for(int i = 0; i < MAX_CONNECTION_TRY; i++) {
           SQLHDBC& hDbc = this->getHandle();
           if(hDbc != nullptr) return hDbc;
        }
        return this->err;
    }

    void returnHandle(SQLHDBC &hDbc) {
        this->mutex.lock();
        for(auto connection : pool) {
            if(connection->getHandle() == hDbc) {
                connection->setState(available);
                break;
            }
        }
        this->mutex.unlock();
    }

    string getQueryResult(int ret) {
        switch(ret) {
            case SQL_SUCCESS:
                return "SQL_SUCCESS";
            case SQL_SUCCESS_WITH_INFO:
                return "SQL_SUCCESS_WITH_INFO";
            case SQL_INVALID_HANDLE:
                return "SQL_INVALID_HANDLE";
            case SQL_ERROR:
                return "SQL_ERROR";
            case SQL_NO_DATA_FOUND:
                return "SQL_NO_DATA_FOUND";
        }
    }

    int executeQuery(SQLHDBC &hDbc, string query) {
        SQLHSTMT        hStmt   = NULL;

        int             cnt = 0;
        int             errCode;
        char            msg[MAX_ERROR_MSG];

        if(SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != 0) {
            DBError(this->hEnv, hDbc, hStmt, msg, errCode);
            LOG_ERROR(std::string(msg) + "code:" + std::to_string(errCode));
            return -1;
        }

        if(SQLExecDirect(hStmt, (SQLCHAR *)query.c_str(), SQL_NTS) != 0) {
            DBError(this->hEnv, hDbc, hStmt, msg, errCode);
            LOG_ERROR(std::string(msg) + ", code:" + std::to_string(errCode));
            cnt = errCode == 1062 ? 0 : -1;
        }

        if(hStmt) {
            if(SQLFreeHandle(SQL_HANDLE_STMT, hStmt) != 0) {
                DBError(this->hEnv, hDbc, hStmt, msg, errCode);
                LOG_ERROR(std::string(msg) + "code:" + std::to_string(errCode));
            }
        }

        return cnt;
    }

    void commit(SQLHDBC &hDbc) {
        (void)SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    }

    void rollback(SQLHDBC& hDbc) {
        (void)SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_ROLLBACK);
    }

    void checkConnection() {
        vector<Connection *> disconnectedList;
        this->mutex.lock();
        for(auto connection : pool) {
            if(connection->getState() == available) {
                if(executeQuery(connection->getHandle(), "select 1") < 0) {
                    connection->setState(disconnected);
                }
            }
            if(connection->getState() == disconnected) {
                disconnectedList.push_back(connection);
            }
        }
        this->mutex.unlock();

        for(auto connection : disconnectedList) {
            if(connection->reconnect(this->hEnv, this->dsn)) connection->setState(available);
        }
    }

    void checkThread() {
        struct timespec tv;
        tv.tv_sec  = 60 * 10;
        tv.tv_nsec = 0;

        while(1) {
            nanosleep(&tv, NULL);
            checkConnection();
        }
    }

    void printInfo() {
        cout << this->dsn << " max-connection:" << this->maxConn << endl;
    }    

    string getError(SQLHDBC &hDbc, SQLHSTMT& hStmt) {
        char    msg[MAX_ERROR_MSG] = {0,};
        int     errCode;
        DBError(this->hEnv, hDbc, hStmt, msg, errCode);
        return "code:" + std::to_string(errCode) + ", " + "msg:" + std::string(msg);
    }

};

















#endif
