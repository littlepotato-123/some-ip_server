#include "conn.h"
using namespace std;

//const char* Conn::srcDir;
std::atomic<int> Conn::userCount;
bool Conn::isET;

Conn::Conn(int fd, const sockaddr_in& addr)  {
    init(fd, addr);
}

Conn::~Conn() {
    --userCount;
    close(fd_);
    //Close();
}

// bool Conn::GetIsClosed() {
//     return isClose_;
// }
void Conn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    ++userCount;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    //isClose_ = false;
}
// void Conn::Close() {
//     isClose_ = true;
// }

int Conn::GetFd() const {
    return fd_;
}

struct sockaddr_in Conn::GetAddr() const {
    return addr_;
}

const char* Conn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int Conn::GetPort() const {
    return ntohs(addr_.sin_port);
}

ssize_t Conn::read(int* saveErrono){
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrono);
        if(len <= 0) break;
    }while(isET);
    return len;
}

ssize_t Conn::write(int* saveErrno) {
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

void Conn::process() {
    while(readBuff_.ReadableBytes() >= 16){
        std::cout<<readBuff_.ReadableBytes()<<std::endl;
        stringstream ss;
        //SqlConnRAII my_sqlConn(&sql, SqlConnPool::Instance());
        std::string query;
        someip_parse_.parse(readBuff_);
        ss << "insert into someip(ServiceId, MethodId, Length, ClientId , SessionId ,MessageType ,PayLoad) values(" << someip_parse_.service_id << "," << someip_parse_.method_id << "," <<someip_parse_.length << "," << someip_parse_.client_id << "," << someip_parse_.session_id << "," <<someip_parse_.message_type << ",'" << someip_parse_.PayLoad_real << "')";
        getline(ss, query);
        std::cout<<"准备放进去"<<std::endl;
        std::cout<<query<<std::endl;
        sqlQueryQueue::Instance()->AddQueue(std::move(query));
        std::cout<<"放进去了"<<std::endl;
        std::cout<<readBuff_.ReadableBytes()<<std::endl;
    }
    readBuff_.RetrieveAll();        
}

    
