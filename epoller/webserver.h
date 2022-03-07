 
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ctime>
#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <cassert>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"
#include "../http/httpconn.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"
class WebServer {
public:
    WebServer( int port, int trigMode, int timeoutMS,bool OptLinger, int sqlPort, const char* sqlUser, 
        const char* sqlPwd, const char* dbName, int connPoolNum, int threadNum );

    ~WebServer();
    void Start();

private:

    void InitEventMode_(int trigMode);
    
    bool InitSocket_(); //socket setsockopt bind listen 

    void AddClient_(int fd, sockaddr_in addr);  //epoll_add
   
    void DealListen_();    //accept
    void DealWrite_(HttpConn* client); //加入线程池的任务队列
    void DealRead_(HttpConn* client);  //加入线程池的任务队列

    void SendError_(int fd, const char*info); //断开连接（连接满了）
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client); //关闭连接，关闭文件描述符，关闭http连接
    
    void OnRead_(HttpConn* client); //任务队列中具体内容
    void OnWrite_(HttpConn* client);//任务队列中具体内容
    void OnProcess(HttpConn* client);//先看能不能读，不可以就转写

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;  /* 毫秒MS */
    bool isClose_;
    int listenFd_;
    //char* srcDir_;
    
    uint32_t listenEvent_;
    uint32_t connEvent_;
   
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};


#endif //WEBSERVER_H