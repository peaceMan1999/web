#pragma once

#include <pthread.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

#include "Log.hpp"

#define PORT 8081
#define BACKLOG 5

class TcpServer{
  private:
    int port;
    int listen_sock;
    static TcpServer* srv;

  private:
    // 设计成单例模式
    TcpServer(int _port = PORT)
        :port(_port)
        ,listen_sock(-1)
    {}

    TcpServer(const TcpServer &s) = delete;
    
  public:

    static TcpServer *getinstance(int port){
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        if (srv == nullptr){
            // 线程锁
            pthread_mutex_lock(&lock);
            if (srv == nullptr){
                srv = new TcpServer(port);
                srv->InitServer(); 
                LOG(INFO, "TcpServer init success!");
            }
            pthread_mutex_unlock(&lock);
        }
        return srv;
    }

    void InitServer()
    {
        Socket();
        Bind();
        Listen();
    }

    void Socket()
    {
        listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock < 0){
            LOG(FATAL, "Socket error >_<");
            exit(1);
        }
        // 复用套接字，避免用户TIME_WAIT
        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        LOG(INFO, "Socket ^_^ success");
    }

    int Sock(){
        return listen_sock;
    }

    void Bind()
    {
        sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(port);
        local.sin_addr.s_addr = INADDR_ANY;
        if (bind(listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0){
            LOG(FATAL, "Bind error >_<");
            exit(2);
        }
        LOG(INFO, "Bind ^_^ success");
    }

    void Listen()
    {
        if (listen(listen_sock, BACKLOG) < 0){
            LOG(FATAL, "Listen error >_<");
            exit(3);
        }
        LOG(INFO, "Listen ^_^ success");
    }

    void Accept()
    {
        struct sockaddr_in peer;
        memset(&peer, 0, sizeof(peer));
        socklen_t len = sizeof(peer);
        if (accept(listen_sock, (struct sockaddr*)&peer, &len) < 0){
            LOG(FATAL, "Accept error >_<");
            exit(4);
        }
        LOG(INFO, "Accept ^_^ success");
    }

    ~TcpServer()
    {
        if (listen_sock >= 0){
            close(listen_sock);
            LOG(INFO, "close sock ^_^ success");
        }
    }
};

// 静态变量要在类外初始化
TcpServer* TcpServer::srv = nullptr;

