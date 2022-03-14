#include <iostream>
#include <string>
#include <queue>
#include <condition_variable>
#include <mutex>
#include "./sqlconnpool.h"
#include <thread>
#include <future>
#include <mysql/mysql.h>

class sqlQueryQueue{
public:
    static sqlQueryQueue* Instance() {
        return &sqlQueryQueue_;
    }

    void AddQueue (std::string query) {
        {
            std::unique_lock<std::mutex> locker(mtx_);
            while(Queries.size() >= maxSize_) {
                notFull_.wait(locker);
            }
            if(isClosed) return;
            Queries.push(std::move(query));
        }
        notEmpty_.notify_one();
    }

    ~sqlQueryQueue() {
        {
            std::lock_guard<std::mutex> locker(mtx_);
            isClosed = true;
        }
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

private:
    static sqlQueryQueue sqlQueryQueue_;

    explicit sqlQueryQueue(size_t maxSize = 1000) : isClosed(false), maxSize_(maxSize) {
        std::thread( [&] () {
            while(1) {
                std::string query;
                {
                    std::unique_lock<std::mutex> locker(mtx_);
                    while(Queries.empty()) {
                        notEmpty_.wait(locker);
                    }
                    if(isClosed) break;
                    query = Queries.front();
                    Queries.pop();
                }
                notFull_.notify_one();
                mysql_query(SqlConnPool::Instance()->sql_conn, query.data());
            }
        } ).detach();
    }

    bool isClosed;
    std::mutex mtx_;
    size_t maxSize_;
    std::queue<std::string> Queries;
    std::condition_variable notFull_, notEmpty_;
};

