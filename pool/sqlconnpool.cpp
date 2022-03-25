#include "sqlconnpool.h"
using namespace std;


SqlConnPool SqlConnPool::sqlconnpool_;

SqlConnPool* SqlConnPool::Instance(){
    return &sqlconnpool_;
}


void SqlConnPool::Init(const char* host, int port,
            const char* user, const char* pwd, const char* dbName) 
{
    //assert(connSize > 0);
    //for(int i = 0; i < connSize; ++i) {
        //MYSQL *sql = nullptr;
        sql_conn = mysql_init(sql_conn);
        if(!sql_conn) {
             printf("mysql init error!\n");
            assert(sql_conn);
        }
        sql_conn = mysql_real_connect(sql_conn, host, user, pwd, dbName, port, nullptr, 0);
        if(!sql_conn){
            printf("mysql connect error!\n");
        }
        
        //connQue_.push(sql);
    //}
    //max_size = connSize;
}

// MYSQL* SqlConnPool::GetConn() {

//     if(is_closing) return nullptr;

//     MYSQL *sql = nullptr;
//     unique_lock<mutex> locker(mtx_);
//     while(connQue_.empty()) {
//         //printf("sqlconnPool busy!\n");
//         not_empty_.wait(locker);
//     }    
//     sql = connQue_.front();
//     connQue_.pop();
//     return sql;
// }

// void SqlConnPool::FreeConn(MYSQL* sql) {
//     if(!sql) {
//         //printf("realeasing a nullptr sql");
//         return;
//     }
//     {
//         lock_guard<mutex> locker(mtx_);
//         connQue_.push(sql);
//     }
//     not_empty_.notify_one();
//     if(connQue_.size() == max_size) {
//         full_.notify_one();
//     }
// }

void SqlConnPool::ClosePool() {
    //unique_lock<mutex> locker(mtx_);
    is_closing = 1;
    // while(connQue_.size() != max_size){
    //     //printf("wait for all pools back\n");
    //     full_.wait(locker);
    // }
    //printf("sqlconnpool is closing\n");
    //int closed_pool_cnt = 0;
    //while(!connQue_.empty()) {
        // auto item = connQue_.front();
        // connQue_.pop();
        mysql_close(sql_conn);
        ////printf("%d sqlconn is closed\n", ++closed_pool_cnt);
    //}
    mysql_library_end();
    //printf("finish closing sqlconnpool\n");
}

// int SqlConnPool::GetFreeConnCount() {
//     lock_guard<mutex> locker(mtx_);
//     return connQue_.size();
// }

SqlConnPool::~SqlConnPool() {
    ClosePool();
}