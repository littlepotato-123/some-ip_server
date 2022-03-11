#include "httpconn.h"
using namespace std;

//const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn(int fd, const sockaddr_in& addr): isClose_(false) {
    init(fd, addr);
}

HttpConn::~HttpConn() {
    --userCount;
    close(fd_);
    Close();
}

bool HttpConn::GetIsClosed() {
    return isClose_;
}
void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    ++userCount;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    //isClose_ = false;
}
void HttpConn::Close() {
    isClose_ = true;
}

int HttpConn::GetFd() const {
    return fd_;
}

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return ntohs(addr_.sin_port);
}

ssize_t HttpConn::read(int* saveErrono){
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrono);
        if(len <= 0) break;
    }while(isET);
    return len;
}

ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if(len < 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0) break; //传输结束    
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t* )iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    }while(isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process() {
    if(readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    else if(!someip_parse_.parse(readBuff_)) {
        return false;
    }
    else {
        MYSQL* sql;
        char oreder[256];
        {
            SqlConnRAII my_sqlConn(&sql, SqlConnPool::Instance());
            if(sql){
                snprintf(oreder, 256, "insert into someip(ServiceId, MethodId, Length, ClientId , SessionId ,MessageType ,PayLoad) values('%u', '%u','%u', '%u','%u', '%u','%s')", someip_parse_.service_id, someip_parse_.method_id,
                        someip_parse_.length,  someip_parse_.client_id, someip_parse_.session_id, someip_parse_.message_type, someip_parse_.PayLoad);
                if(mysql_query(sql, oreder)) {
                    //printf("%s failed to add to sql", line.data());
                }
            } 
        }
    }

    // else {
    //     std::string line((char* )readBuff_.Peek(), readBuff_.ReadableBytes());
    //     readBuff_.RetrieveAll();

    //     auto t = time(nullptr);
    //     tm now_time;
    //     localtime_r(&t, &now_time);
    //     char time_buf[64];
    //     snprintf(time_buf, 64, "%d-%d-%d %d:%d:%d\n", now_time.tm_year + 1900, now_time.tm_mon + 1,
    //             now_time.tm_mday, now_time.tm_hour, now_time.tm_min, now_time.tm_sec );
        
    //     MYSQL* sql;
    //     char oreder[256];
    //     {
    //         SqlConnRAII my_sqlConn(&sql, SqlConnPool::Instance());
    //         if(sql){
    //             snprintf(oreder, 256, "insert into user(username, password) values('%s', '%s')",time_buf , line.data());
    //             if(mysql_query(sql, oreder)) {
    //                 printf("%s failed to add to sql", line.data());
    //             }
    //         } 
    //     }
    //     transform(line.begin(), line.end(), line.begin(), ::toupper);
    //     writeBuff_.Append(line.data(), line.size());
    //     string mark("this bill's web");
    //     writeBuff_.Append(mark.data(), mark.size());
    //     iov_[0].iov_base = const_cast<char* >((char* )writeBuff_.Peek());
    //     iov_[0].iov_len = writeBuff_.ReadableBytes();
    //     iovCnt_ = 1;
    // }
    return true;
}