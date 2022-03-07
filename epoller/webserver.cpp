#include "webserver.h"
using namespace std;

WebServer::WebServer(int port, int trigMode, int timeoutMS,bool OptLinger, int sqlPort, const char* sqlUser, 
        const char* sqlPwd, const char* dbName, int connPoolNum, int threadNum):port_(port), timeoutMS_(timeoutMS),
        openLinger_(OptLinger), isClose_(false),threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller()), timer_(new HeapTimer())
{
    //srcDir_ = getcwd(nullptr, 256);
    //assert(srcDir_);
    //strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    //HttpConn::srcDir = srcDir_;

    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode_(trigMode);
    if(!InitSocket_()) { isClose_ = true;}
}

WebServer::~WebServer() {
    printf("webserver is disconstructing\n");
    close(listenFd_);
    isClose_ = true;
    //free(srcDir_);
    //SqlConnPool::Instance()->ClosePool();
}

void WebServer::InitEventMode_(int trigMode) {
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
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::Start(){
    int timeMS = -1;
    //auto begin_time = clock();
    while(!isClose_){
        
        // auto now_time = clock();
        // auto time_pass = (double)(now_time - begin_time)/CLOCKS_PER_SEC * 1000;
        // printf("%f\n", time_pass);

        // if( time_pass  >= 1){ 
        //     printf("time out\n");
        //     isClose_ = 1;
        //     continue;
        // }
        if(timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
            printf("timeMS = %d\n", timeMS);
        }

        int eventCnt = epoller_->Wait(timeMS);
        printf("one more loop\n");

        for(int i = 0; i < eventCnt; ++i){
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) {
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            } 
        }
    }
}

void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    send(fd, info, strlen(info), 0);
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    printf("closing client:%d\n ip:%s port:%d\n",client->GetFd(), client->GetIP(), client->GetPort());
    SendError_(client->GetFd(), "time out!");
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
}

void WebServer::DealListen_(){
    printf("deal listen\n");
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do{
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0){return ;}
        else if(HttpConn::userCount >= MAX_FD){
            SendError_(fd, "Server busy!");
            return;
        }
        printf("deal listen new connect fd = %d\n", fd);
        AddClient_(fd, addr);
        printf("deal listen addclient over\n");
    }while(listenEvent_ & EPOLLET); 
}

void WebServer::DealRead_(HttpConn* client){

    assert(client);
    printf("start to deal read\n");
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    printf("start to deal write\n");
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}
void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    printf("onReading\n");
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    printf("onRead over ,begin to on process\n");
    OnProcess(client);
}

void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(1) {
            OnProcess(client);
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
    CloseConn_(client);
}
bool WebServer::InitSocket_(){
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

    ret = listen(listenFd_, 6);
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

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    printf("extentTiming\n");
    if(timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
    printf("extentTime over\n");
}