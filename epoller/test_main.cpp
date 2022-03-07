#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "epoller.h"
#include <iostream>
#include <string>
using namespace std;
int main(int argc, char *argv[]){
    char buf[BUFSIZ] = {0};
    Epoller my_epoll;
    int lfd, cfd;
    struct sockaddr_in s_addr, c_addr;
    socklen_t c_addr_len = sizeof(c_addr);
    bzero(&s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(9527);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(lfd, (struct sockaddr *)&s_addr, sizeof(s_addr));
    listen(lfd, 128);
    cfd = accept(lfd, (struct sockaddr *)&c_addr, &c_addr_len);
    my_epoll.AddFd(cfd, EPOLLIN);
    while(1){
        
        int n;
        int nfd = my_epoll.Wait();
        if(my_epoll.GetEvents(0) & EPOLLIN){
            int fd = my_epoll.GetEventFd(0);
            n = read(fd, buf, BUFSIZ);
            if(n <= 0)break;
            cout<<"读到客户端数据，即将显示在下面\n";
            write(STDOUT_FILENO, buf, n);
            cout<<endl;
            my_epoll.ModFd(fd, EPOLLOUT);
        }
        else {
            int fd = my_epoll.GetEventFd(0);
            for(int i = 0; i < n ; ++i){
                buf[i] = toupper(buf[i]);
            }
            write(fd, buf, n);
            cout<<endl;
            cout<<"写给客户端的\n";
            buf[n] = '\0';
            cout<<string(buf)<<endl;
            bzero(buf, sizeof(buf));
            my_epoll.ModFd(fd, EPOLLIN);
        }
    }
    

}