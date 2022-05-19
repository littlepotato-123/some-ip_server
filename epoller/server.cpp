#include "server.h"
using namespace std;

Server::Server(int port, int trigMode, bool OptLinger, int sqlPort, const char* sqlUser, 
        const char* sqlPwd, const char* dbName, int connPoolNum, int threadNum):port_(port), 
        openLinger_(OptLinger), isClose_(false),threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
    Conn::userCount = 0;

    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName);
    
    InitEventMode_(trigMode);
    if(!InitSocket_()) { isClose_ = true;}
}

Server::~Server() {

    close(listenFd_);
    isClose_ = true;
}

void Server::InitEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    Conn::isET = (connEvent_ & EPOLLET);
}

void Server::Start(){
    int timeMS = -1;
    while(!isClose_){
        
        int eventCnt = epoller_->Wait(timeMS);

        for(int i = 0; i < eventCnt; ++i){
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) {
                DealListen_();//<-----
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                //assert(users_.count(fd) > 0);
                CloseConn_(weak_ptr<Conn>(users_[fd]));
            }
            else if(events & EPOLLIN) {
                //assert(users_.count(fd) > 0);
                DealRead_(weak_ptr<Conn>(users_[fd]));
            }
            else if(events & EPOLLOUT) {
               // assert(users_.count(fd) > 0);
                DealWrite_(weak_ptr<Conn>(users_[fd]));
            } 
        }
    }
}

void Server::SendError_(int fd, const char*info) {
    assert(fd > 0);
    send(fd, info, strlen(info), 0);
    close(fd);
}

void Server::CloseConn_(weak_ptr<Conn> wkclient) {
    //assert(client);
    int fd ;
    if(auto client = wkclient.lock()){
        fd = client->GetFd();
    }
    users_.erase(fd);
    epoller_->DelFd(fd); 
}

void Server::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    SetFdNonblock(fd);
    users_[fd] = make_shared<Conn>(fd, addr);
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    
}

void Server::DealListen_(){
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do{
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0){
            //std::cout<<"fd小于零accept"<<std::endl;
            return ;
            }
        else if(Conn::userCount >= MAX_FD){
            std::cout<<"fd大于最大accept"<<std::endl;
            SendError_(fd, "Server busy!");
            return;
        }
        AddClient_(fd, addr);
    }while(listenEvent_ & EPOLLET); 
}

void Server::DealRead_(weak_ptr<Conn> wkclient){

    if(auto client = wkclient.lock()){
        threadpool_->AddTask(std::bind(&Server::OnRead_, this, weak_ptr<Conn>(client)));
    }
}

void Server::DealWrite_(weak_ptr<Conn> wkclient) {
   // assert(client);
    if(auto client = wkclient.lock()){
        threadpool_->AddTask(std::bind(&Server::OnWrite_, this, weak_ptr<Conn>(client)));
    }
}
void Server::OnRead_(weak_ptr<Conn> wkclient) {
    //assert(client);
    int ret = -1;
    int readErrno = 0;
    {
        auto client = wkclient.lock();
        if(!client) return;
        ret = client->read(&readErrno);
    }
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(wkclient);
        return;
    }
    OnProcess(wkclient);
}

void Server::OnProcess(weak_ptr<Conn> wkclient) {
    auto client = wkclient.lock();
    if(!client) return;
    client->process();
    epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    
}

void Server::OnWrite_(weak_ptr<Conn> wkclient) {
    auto client = wkclient.lock();
    if(!client) return;
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(1) {
            OnProcess(weak_ptr<Conn>(client));
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(weak_ptr<Conn>(client));
}
bool Server::InitSocket_(){
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024){
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = {0};
    if(openLinger_){
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0){
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0){
        close(listenFd_);
        return false;
    }

    int optval = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    if(ret == -1){
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0){
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 128);
    if(ret < 0){
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if(ret == 0){
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    return true;
}

int Server::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
