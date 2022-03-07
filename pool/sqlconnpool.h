/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <condition_variable>
#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <assert.h>

class SqlConnPool {
public:
    static SqlConnPool* Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();
    

private:
    SqlConnPool() = default;
    ~SqlConnPool();

    int max_size = 0;
    bool is_closing = false;
    
    std::queue<MYSQL *> connQue_;
    std::condition_variable full_, not_empty_;
    std::mutex mtx_;
    //sem_t semId_;
};


#endif // SQLCONNPOOL_H